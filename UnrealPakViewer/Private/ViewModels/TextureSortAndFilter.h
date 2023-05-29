#pragma once

#include "Async/AsyncWork.h"
#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "Stats/Stats.h"

#include "PakFileEntry.h"

DECLARE_DELEGATE_ThreeParams(FOnSortAndFilterFinished, const FName, EColumnSortMode::Type, const FString&);

class SPakTextureView;

class FTextureSortAndFilterTask : public FNonAbandonableTask
{
public:
	FTextureSortAndFilterTask(FName InSortedColumn, EColumnSortMode::Type InSortMode, TSharedPtr<SPakTextureView> InPakFileView)
		: CurrentSortedColumn(InSortedColumn)
		, CurrentSortMode(InSortMode)
		, CurrentSearchText(TEXT(""))
		, WeakPakFileView(InPakFileView)
	{

	}

	void DoWork();
	void SetWorkInfo(FName InSortedColumn, EColumnSortMode::Type InSortMode, const FString& InSearchText, const TMap<FName, bool>& InClassFilterMap, const TMap<int32, bool>& InIndexFilterMap);
	FOnSortAndFilterFinished& GetOnSortAndFilterFinishedDelegate() { return OnWorkFinished; }

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(STAT_FFileSortAndFilterTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	void RetriveResult(TArray<FPakFileEntryPtr>& OutResult);

protected:
	FName CurrentSortedColumn;
	EColumnSortMode::Type CurrentSortMode;
	FString CurrentSearchText;

	/** Shared pointer to parent PakFileView widget. Used for accesing the cache and to check if cancel is requested. */
	TWeakPtr<SPakTextureView> WeakPakFileView;

	FOnSortAndFilterFinished OnWorkFinished;

	FCriticalSection CriticalSection;
	TArray<FPakFileEntryPtr> Result;

	TMap<FName, bool> ClassFilterMap;
	TMap<int32, bool> IndexFilterMap;
};