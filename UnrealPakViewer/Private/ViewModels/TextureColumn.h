#pragma once

#include "CoreMinimal.h"

#include "PakFileEntry.h"

#include "FileColumn.h"

class FTextureColumn
{
public:
	typedef TFunction<bool(const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B)> FTextureCompareFunc;

	static const FName NameColumnName;
	static const FName PathColumnName;
	static const FName SizeColumnName;
	static const FName WidthColumnName;
	static const FName HeightColumnName;
	static const FName MipGenSettingColumnName;
	static const FName LodBiasColumnName;
	static const FName MaxSizeColumnName;
	static const FName GroupColumnName;
	static const FName IsStreamingColumnName;
	static const FName IsSRGBColumnName;
	static const FName CompressionColumnName;

	FTextureColumn() = delete;
	FTextureColumn(int32 InIndex, const FName InId, const FText& InTitleName, const FText& InDescription, float InFillWidth, const EFileColumnFlags& InFlags)
		: Index(InIndex)
		, Id(InId)
		, TitleName(InTitleName)
		, Description(InDescription)
		, FillWidth(InFillWidth)
		, Flags(InFlags)
	{
	}

	int32 GetIndex() const { return Index; }
	const FName& GetId() const { return Id; }
	const FText& GetTitleName() const { return TitleName; }
	const FText& GetDescription() const { return Description; }

	bool IsVisible() const { return bIsVisible; }
	void Show() { bIsVisible = true; }
	void Hide() { bIsVisible = false; }
	void ToggleVisibility() { bIsVisible = !bIsVisible; }
	void SetVisibilityFlag(bool bOnOff) { bIsVisible = bOnOff; }

	float GetFillWidth() const { return FillWidth; }

	/** Whether this column should be initially visible. */
	bool ShouldBeVisible() const { return EnumHasAnyFlags(Flags, EFileColumnFlags::ShouldBeVisible); }

	/** Whether this column can be hidden. */
	bool CanBeHidden() const { return EnumHasAnyFlags(Flags, EFileColumnFlags::CanBeHidden); }

	/** Whether this column can be used for filtering displayed results. */
	bool CanBeFiltered() const { return EnumHasAnyFlags(Flags, EFileColumnFlags::CanBeFiltered); }

	/** Whether this column can be used for sort displayed results. */
	bool CanBeSorted() const { return AscendingCompareDelegate && DescendingCompareDelegate; }
	void SetAscendingCompareDelegate(FTextureCompareFunc InCompareDelegate) { AscendingCompareDelegate = InCompareDelegate; }
	void SetDescendingCompareDelegate(FTextureCompareFunc InCompareDelegate) { DescendingCompareDelegate = InCompareDelegate; }

	FTextureCompareFunc GetAscendingCompareDelegate() const { return AscendingCompareDelegate; }
	FTextureCompareFunc GetDescendingCompareDelegate() const { return DescendingCompareDelegate; }

protected:
	int32 Index;
	FName Id;
	FText TitleName;
	FText Description;
	float FillWidth;

	EFileColumnFlags Flags;
	FTextureCompareFunc AscendingCompareDelegate;
	FTextureCompareFunc DescendingCompareDelegate;

	bool bIsVisible;
};