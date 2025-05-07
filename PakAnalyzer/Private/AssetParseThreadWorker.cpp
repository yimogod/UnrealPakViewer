#include "AssetParseThreadWorker.h"

#include "AssetRegistry/PackageReader.h"
#include "Async/ParallelFor.h"
#include "HAL/CriticalSection.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/RunnableThread.h"
#include "IPlatformFilePak.h"
#include "Misc/Compression.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Serialization/Archive.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "UObject/ObjectResource.h"
#include "UObject/ObjectVersion.h"
#include "UObject/PackageFileSummary.h"

#include "CommonDefines.h"
#include "ExtractThreadWorker.h"


FAssetParseThreadWorker::FAssetParseThreadWorker() : Thread(nullptr)
{
}

FAssetParseThreadWorker::~FAssetParseThreadWorker()
{
	Shutdown();
}

bool FAssetParseThreadWorker::Init()
{
	return true;
}

uint32 FAssetParseThreadWorker::Run()
{
	UE_LOG(LogPakAnalyzer, Display, TEXT("Asset parse worker starts."));

	FCriticalSection Mutex;
	const static bool bForceSingleThread = false;
	const int32 TotalCount = Files.Num();
	
	TMultiMap<FName, FName> DependsMap;
	TMap<FName, FName> ClassMap;

	// Parse assets
	ParallelFor(TotalCount, [this, &DependsMap, &ClassMap, &Mutex](int32 InIndex){
		if (StopTaskCounter.GetValue() > 0)return;

		FPakFileEntryPtr File = Files[InIndex];
		if (!Summaries.IsValidIndex(File->OwnerPakIndex) || File->PakEntry.IsDeleteRecord())
		{
			return;
		}

		const FPakFileSumary& Summary = Summaries[File->OwnerPakIndex];
		const FString PakFilePath = Summary.PakFilePath;


		FString PackagePath = File->Path;
		FPackageReader::EOpenPackageResult ErrorCode;
		FPackageReader Reader;
		
		FPaths::NormalizeFilename(PackagePath);
		Reader.OpenPackageFile(FStringView(PackagePath), &ErrorCode);


		//读取package成功
		if (ErrorCode == FPackageReader::EOpenPackageResult::Success)
		{
			FString RootPackageName = Reader.GetLongPackageName();
			
			// 创建 FAssetSummary 数据
			File->AssetSummary = MakeShared<FAssetSummary>();
			File->AssetSummary->PackageSummary = Reader.GetPackageFileSummary();

			//获取 names 数据
			TArray<FName> Names;
			Reader.GetNames(Names);
			for (FName Name : Names)
			{
				File->AssetSummary->Names.Add(MakeShared<FName>(Name));
			}

			FLinkerTables Tables;
			auto PackageIndexToObjectPath = [&Tables, &RootPackageName](FPackageIndex Index) -> FSoftObjectPath
			{
				if (Index.IsNull())
				{
					return FSoftObjectPath{};
				}

				return Index.IsExport() 
					? Tables.GetExportPathName(RootPackageName, Index.ToExport()) 
					: Tables.GetImportPathName(Index.ToImport());
			};
			
			// 获取 import 数据
			if (Reader.GetImports(Tables.ImportMap))
			{
				for (int32 ImportIndex = 0; ImportIndex < Tables.ImportMap.Num(); ++ImportIndex)
				{
					FObjectImport& Import = Tables.ImportMap[ImportIndex];
					FSoftObjectPath ImportPathName = Tables.GetImportPathName(ImportIndex);
					
					FObjectImportPtrType ImportEx = MakeShared<FObjectImportEx>();
					ImportEx->Index = ImportIndex;
					ImportEx->ObjectName = Import.ObjectName;
					ImportEx->ClassPackage = Import.ClassPackage;
					ImportEx->ClassName = Import.ClassName;

					ImportEx->ObjectPath = Import.ObjectName;
				
					File->AssetSummary->ObjectImports.Add(ImportEx);
				}
			}
			else
			{
				UE_LOG(LogPakAnalyzer, Error, TEXT("Error reading import table for package file %s"), *PackagePath);
			}

			//获取 export 数据
			if (Reader.GetExports(Tables.ExportMap))
			{
				for (int32 ExportIndex = 0; ExportIndex < Tables.ExportMap.Num(); ++ExportIndex)
				{
					FObjectExport& Export = Tables.ExportMap[ExportIndex];
					FSoftObjectPath ExportPathName = Tables.GetExportPathName(RootPackageName, ExportIndex);
					FSoftObjectPath ClassPathName = PackageIndexToObjectPath(Export.ClassIndex);
					
					FObjectExportPtrType ExportEx = MakeShared<FObjectExportEx>();
					ExportEx->Index = ExportIndex;
					ExportEx->ObjectName = Export.ObjectName;
					ExportEx->ObjectPath = *ExportPathName.ToString();
					ExportEx->ClassName = *ClassPathName.ToString();
					
					ExportEx->SerialSize = Export.SerialSize;
					ExportEx->SerialOffset = Export.SerialOffset;
					ExportEx->bIsAsset = Export.bIsAsset;
					ExportEx->bNotForClient = Export.bNotForClient;
					ExportEx->bNotForServer = Export.bNotForServer;
				
					File->AssetSummary->ObjectExports.Add(ExportEx);
				}
			}
			else
			{
				UE_LOG(LogPakAnalyzer, Error, TEXT("Error reading export table for package file %s"), *PackagePath);
			}

			//获取depend数据
			if (Reader.GetDependsMap(Tables.DependsMap))
			{
				for (int32 ExportIndex = 0; ExportIndex < Tables.ExportMap.Num(); ++ExportIndex)
				{
					FString ExportPath = PackageIndexToObjectPath(FPackageIndex::FromExport(ExportIndex)).ToString();
					int32 NumDepends = Tables.DependsMap[ExportIndex].Num();
					if (NumDepends == 0)continue;
					
					for (int32 DependsIndex = 0; DependsIndex < NumDepends; ++DependsIndex)
					{
						FSoftObjectPath DependsPath = PackageIndexToObjectPath(Tables.DependsMap[ExportIndex][DependsIndex]);

						FPackageInfoPtr Depends = MakeShared<FPackageInfo>();
						Depends->PackageName = *DependsPath.ToString();
	
						File->AssetSummary->DependencyList.Add(Depends);
					}
				}
			}
			else
			{
				UE_LOG(LogPakAnalyzer, Error, TEXT("Error reading depends map for package file %s"), *PackagePath);
			}
			
			// if (!Reader.GetSoftPackageReferenceList(Tables.SoftPackageReferenceList))
			// {
			// 	UE_LOG(LogPakAnalyzer, Error, TEXT("Error reading soft package reference list for package file %s"), *PackagePath);
			// }
		}
	}, bForceSingleThread);

	OnParseFinish.ExecuteIfBound(StopTaskCounter.GetValue() > 0, ClassMap);

	StopTaskCounter.Reset();

	UE_LOG(LogPakAnalyzer, Display, TEXT("Asset parse worker exits."));

	return 0;
}

void FAssetParseThreadWorker::Stop()
{
	StopTaskCounter.Increment();
	EnsureCompletion();
	StopTaskCounter.Reset();
}

void FAssetParseThreadWorker::Exit()
{

}

void FAssetParseThreadWorker::Shutdown()
{
	Stop();

	if (Thread)
	{
		UE_LOG(LogPakAnalyzer, Log, TEXT("Shutdown asset parse worker."));

		delete Thread;
		Thread = nullptr;
	}
}

void FAssetParseThreadWorker::EnsureCompletion()
{
	if (Thread)
	{
		Thread->WaitForCompletion();
	}
}

void FAssetParseThreadWorker::StartParse(TArray<FPakFileEntryPtr>& InFiles, TArray<FPakFileSumary>& InSummaries)
{
	Shutdown();

	UE_LOG(LogPakAnalyzer, Log, TEXT("Start asset parse worker, file count: %d."), InFiles.Num());

	Files = MoveTemp(InFiles);
	Summaries = MoveTemp(InSummaries);

	Thread = FRunnableThread::Create(this, TEXT("AssetParseThreadWorker"), 0, EThreadPriority::TPri_Highest);
}
