#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "Misc/AES.h"

#include "Misc/Guid.h"
#include "PakFileEntry.h"

typedef TMap<FName, FName> ClassTypeMap;
DECLARE_DELEGATE_TwoParams(FOnParseFinish, bool/* bCancel*/, const ClassTypeMap&/* ClassMap*/);

class FAssetParseThreadWorker : public FRunnable
{
public:
	FAssetParseThreadWorker();
	~FAssetParseThreadWorker();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

	void Shutdown();
	void EnsureCompletion();
	void StartParse(TArray<FPakFileEntryPtr>& InFiles, TArray<FPakFileSumary>& InSummaries);

	FOnParseFinish OnParseFinish;

protected:
	class FRunnableThread* Thread;
	FThreadSafeCounter StopTaskCounter;

	//本pak中有几个upackage(uasset和uexp对应一个)
	TArray<FPakFileEntryPtr> Files;
	TArray<FPakFileSumary> Summaries;
};
