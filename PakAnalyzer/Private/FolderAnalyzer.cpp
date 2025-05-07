#include "FolderAnalyzer.h"
#include "AssetParseThreadWorker.h"
#include "CommonDefines.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFile.h"
#include "HAL/PlatformMisc.h"
#include "HAL/UnrealMemory.h"
#include "Json.h"
#include "Misc/Base64.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Serialization/Archive.h"
#include "Serialization/MemoryWriter.h"


FFolderAnalyzer::FFolderAnalyzer()
{
	Reset();
	InitializeAssetParseWorker();
}

FFolderAnalyzer::~FFolderAnalyzer()
{
	ShutdownAssetParseWorker();
	Reset();
}

bool FFolderAnalyzer::LoadPakFiles(const TArray<FString>& InPakPaths, const TArray<FString>& InDefaultAESKeys, int32 ContainerStartIndex)
{
	const FString InPakPath = InPakPaths.Num() > 0 ? InPakPaths[0] : TEXT("");
	if (InPakPath.IsEmpty())
	{
		UE_LOG(LogPakAnalyzer, Error, TEXT("Open folder failed! Folder path is empty!"));
		return false;
	}

	UE_LOG(LogPakAnalyzer, Log, TEXT("Start open folder: %s."), *InPakPath);

	IPlatformFile& PlatformFile = IPlatformFile::GetPlatformPhysical();
	if (!PlatformFile.DirectoryExists(*InPakPath))
	{
		FPakAnalyzerDelegates::OnLoadPakFailed.ExecuteIfBound(FString::Printf(TEXT("Open folder failed! Folder not exists! Path: %s."), *InPakPath));
		UE_LOG(LogPakAnalyzer, Error, TEXT("Open folder failed! Folder not exists! Path: %s."), *InPakPath);
		return false;
	}

	Reset();

	//这里没有真正的pak文件, 所有制造了对应的数据结构 
	FPakFileSumaryPtr Summary = MakeShared<FPakFileSumary>();
	PakFileSummaries.Add(Summary);

	// Save pak sumary
	Summary->MountPoint = InPakPath;
	FMemory::Memset(&Summary->PakInfo, 0, sizeof(Summary->PakInfo));
	Summary->PakFilePath = InPakPath;

	ShutdownAssetParseWorker();

	// Make tree root
	FPakTreeEntryPtr TreeRoot = MakeShared<FPakTreeEntry>(*FPaths::GetCleanFilename(InPakPath), Summary->MountPoint, true);

	//目录下的所有文件, 大部分是uasset和uexp
	//这里保存的都是文件的绝对路径
	TArray<FString> FoundFiles;
	PlatformFile.FindFilesRecursively(FoundFiles, *InPakPath, TEXT(""));

	// 针对每个文件, 造假一个FPakEntry对象
	int64 TotalSize = 0;
	for (const FString& File : FoundFiles)
	{
		FPakEntry Entry;
		Entry.Offset = 0;
		Entry.UncompressedSize = PlatformFile.FileSize(*File);
		Entry.Size = Entry.UncompressedSize;

		TotalSize += Entry.UncompressedSize;

		FString RelativeFilename = File;
		RelativeFilename.RemoveFromStart(InPakPath);

		FPakTreeEntryPtr TreeEnty = InsertFileToTree(TreeRoot, *Summary, RelativeFilename, Entry);
		TreeEnty->Path = File;

		//实际上这个文件的内容不是必须
		//if (File.Contains(TEXT("DevelopmentAssetRegistry.bin")))
		if (File.Contains(TEXT("AssetRegistry.bin")))
		{
			AssetRegistryPath = File;
		}
	}

	Summary->PakFileSize = TotalSize;
	Summary->FileCount = TreeRoot->FileCount;

	RefreshTreeNode(TreeRoot);
	RefreshTreeNodeSizePercent(TreeRoot, TreeRoot);

	PakTreeRoots.Add(TreeRoot);

	if (!AssetRegistryPath.IsEmpty())
	{
		LoadAssetRegistry(AssetRegistryPath);
	}

	ParseAssetFile(TreeRoot);

	UE_LOG(LogPakAnalyzer, Log, TEXT("Finish load pak file: %s."), *InPakPath);

	FPakAnalyzerDelegates::OnPakLoadFinish.Broadcast();

	return true;
}

void FFolderAnalyzer::ExtractFiles(const FString& InOutputPath, TArray<FPakFileEntryPtr>& InFiles)
{
}

void FFolderAnalyzer::CancelExtract()
{
}

void FFolderAnalyzer::SetExtractThreadCount(int32 InThreadCount)
{
}

void FFolderAnalyzer::ParseAssetFile(FPakTreeEntryPtr InRoot)
{
	if (AssetParseWorker.IsValid())
	{
		//拿到所有的uasset和umap文件
		TArray<FPakFileEntryPtr> UAssetFiles;
		RetriveUAssetFiles(InRoot, UAssetFiles);

		TArray<FPakFileSumary> Summaries = { *PakFileSummaries[0] };
		AssetParseWorker->StartParse(UAssetFiles, Summaries);
	}
}

void FFolderAnalyzer::InitializeAssetParseWorker()
{
	UE_LOG(LogPakAnalyzer, Log, TEXT("Initialize asset parse worker."));

	if (!AssetParseWorker.IsValid())
	{
		AssetParseWorker = MakeShared<FAssetParseThreadWorker>();
		AssetParseWorker->OnReadAssetContent.BindRaw(this, &FFolderAnalyzer::OnReadAssetContent);
		AssetParseWorker->OnParseFinish.BindRaw(this, &FFolderAnalyzer::OnAssetParseFinish);
	}
}

void FFolderAnalyzer::ShutdownAssetParseWorker()
{
	if (AssetParseWorker.IsValid())
	{
		AssetParseWorker->Shutdown();
	}
}

void FFolderAnalyzer::OnReadAssetContent(FPakFileEntryPtr InFile, bool& bOutSuccess, TArray<uint8>& OutContent)
{
	bOutSuccess = false;
	OutContent.Empty();

	if (!InFile.IsValid())
	{
		return;
	}

	const FString FilePath = PakFileSummaries[0]->MountPoint / InFile->Path;
	bOutSuccess = FFileHelper::LoadFileToArray(OutContent, *FilePath);
}

void FFolderAnalyzer::OnAssetParseFinish(bool bCancel, const TMap<FName, FName>& ClassMap)
{
	if (bCancel)
	{
		return;
	}

	DefaultClassMap = ClassMap;
	const bool bRefreshClass = ClassMap.Num() > 0;

	FFunctionGraphTask::CreateAndDispatchWhenReady([this, bRefreshClass]()
		{
			if (bRefreshClass)
			{
				for (const FPakTreeEntryPtr& PakTreeRoot : PakTreeRoots)
				{
					RefreshClassMap(PakTreeRoot, PakTreeRoot);
				}
			}

			FPakAnalyzerDelegates::OnAssetParseFinish.Broadcast();
		},
		TStatId(), nullptr, ENamedThreads::GameThread);
}
