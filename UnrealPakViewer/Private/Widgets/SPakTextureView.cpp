#include "SPakTextureView.h"

#include "Async/AsyncWork.h"
#include "Async/TaskGraphInterfaces.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Json.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IPlatformFilePak.h"
#include "Misc/Guid.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STableViewBase.h"

#include "CommonDefines.h"
#include "PakAnalyzerModule.h"
#include "UnrealPakViewerStyle.h"
#include "ViewModels/ClassColumn.h"
#include "ViewModels/FileSortAndFilter.h"
#include "ViewModels/WidgetDelegates.h"

#define LOCTEXT_NAMESPACE "SPakTextureView"

////////////////////////////////////////////////////////////////////////////////////////////////////
// SPakTextureRow
////////////////////////////////////////////////////////////////////////////////////////////////////

class SPakTextureRow : public SMultiColumnTableRow<FPakTextureEntryPtr>
{
	SLATE_BEGIN_ARGS(SPakTextureRow) {}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, FPakTextureEntryPtr InPakFileItem, TSharedRef<SPakTextureView> InParentWidget, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		if (!InPakFileItem.IsValid())
		{
			return;
		}

		WeakPakTextureItem = MoveTemp(InPakFileItem);
		WeakPakFileView = InParentWidget;

		SMultiColumnTableRow<FPakTextureEntryPtr>::Construct(FSuperRowType::FArguments(), InOwnerTableView);

		TSharedRef<SWidget> Row = ChildSlot.GetChildAt(0);

		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(new FSlateColorBrush(FLinearColor::White))
			.BorderBackgroundColor(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f)))
			[
				Row
			]
		];
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (ColumnName == FTextureColumn::NameColumnName)
		{
			return
				SNew(SBox).Padding(FMargin(4.0, 0.0))
				[
					SNew(STextBlock).Text(this, &SPakTextureRow::GetName).ToolTipText(this, &SPakTextureRow::GetName).HighlightText(this, &SPakTextureRow::GetSearchHighlightText)
				];
		}
		else if (ColumnName == FTextureColumn::PathColumnName)
		{
			return
				SNew(SBox).Padding(FMargin(4.0, 0.0))
				[
					SNew(STextBlock).Text(this, &SPakTextureRow::GetPath).ToolTipText(this, &SPakTextureRow::GetPath).HighlightText(this, &SPakTextureRow::GetSearchHighlightText)
				];
		}
		else if (ColumnName == FTextureColumn::SizeColumnName)
		{
			return
				SNew(SBox).Padding(FMargin(4.0, 0.0))
				[
					SNew(STextBlock).Text(this, &SPakTextureRow::GetSize).ToolTipText(this, &SPakTextureRow::GetSizeToolTip)
				];
		}
		else if (ColumnName == FTextureColumn::WidthColumnName)
		{
			return
				SNew(SBox).Padding(FMargin(4.0, 0.0))
				[
					SNew(STextBlock).Text(this, &SPakTextureRow::GetWidth)
				];
		}
		else if (ColumnName == FTextureColumn::HeightColumnName)
		{
			return
				SNew(SBox).Padding(FMargin(4.0, 0.0))
				[
					SNew(STextBlock).Text(this, &SPakTextureRow::GetWidth)
				];
		}
		
		else if (ColumnName == FTextureColumn::MipGenSettingColumnName)
		{
			return
				SNew(SBox).Padding(FMargin(4.0, 0.0))
				[
					SNew(STextBlock).Text(this, &SPakTextureRow::GetMipGen)
				];
		}
		else if (ColumnName == FTextureColumn::LodBiasColumnName)
		{
			return
				SNew(SBox).Padding(FMargin(4.0, 0.0))
				[
					SNew(STextBlock).Text(this, &SPakTextureRow::GetLodBias)
				];
		}
		else if (ColumnName == FTextureColumn::MaxSizeColumnName)
		{
			return
				SNew(SBox).Padding(FMargin(4.0, 0.0))
				[
					SNew(STextBlock).Text(this, &SPakTextureRow::GetMaxSize)
				];
		}
		else if (ColumnName == FTextureColumn::GroupColumnName)
		{
			return
				SNew(SBox).Padding(FMargin(4.0, 0.0))
				[
					SNew(STextBlock).Text(this, &SPakTextureRow::GetTextureGroup)
				];
		}
		else if (ColumnName == FTextureColumn::IsStreamingColumnName)
		{
			return
				SNew(SBox).Padding(FMargin(4.0, 0.0))
				[
					SNew(STextBlock).Text(this, &SPakTextureRow::GetStreaming)
				];
		}
		else if (ColumnName == FTextureColumn::IsSRGBColumnName)
		{
			return
				SNew(SBox).Padding(FMargin(4.0, 0.0))
				[
					SNew(STextBlock).Text(this, &SPakTextureRow::GetsRGB)
				];
		}
		else if (ColumnName == FTextureColumn::CompressionColumnName)
		{
			return
				SNew(SBox).Padding(FMargin(4.0, 0.0))
				[
					SNew(STextBlock).Text(this, &SPakTextureRow::GetTextureCompression)
				];
		}
		else
		{
			return SNew(STextBlock).Text(LOCTEXT("UnknownColumn", "Unknown Column"));
		}
	}

protected:

	FText GetSearchHighlightText() const
	{
		TSharedPtr<SPakTextureView> ParentWidgetPin = WeakPakFileView.Pin();
		
		return ParentWidgetPin.IsValid() ? ParentWidgetPin->GetSearchText() : FText();
	}

	FText GetName() const
	{
		FPakTextureEntryPtr PakFileItemPin = WeakPakTextureItem.Pin();
		if (PakFileItemPin.IsValid())
		{
			return FText::FromName(PakFileItemPin->Filename);
		}
		else
		{
			return FText();
		}
	}

	FText GetPath() const
	{
		FPakTextureEntryPtr PakFileItemPin = WeakPakTextureItem.Pin();
		if (PakFileItemPin.IsValid())
		{
			return FText::FromString(PakFileItemPin->Path);
		}
		else
		{
			return FText();
		}
	}

	FText GetWidth() const
	{
		FPakTextureEntryPtr PakFileItemPin = WeakPakTextureItem.Pin();
		if (PakFileItemPin.IsValid())
		{
			return FText::AsNumber(PakFileItemPin->Width);
		}
		else
		{
			return FText();
		}
	}

	FSlateColor GetClassColor() const
	{
		FPakTextureEntryPtr PakFileItemPin = WeakPakTextureItem.Pin();
		if (PakFileItemPin.IsValid())
		{
			return FSlateColor(FClassColumn::GetColorByClass(*PakFileItemPin->Class.ToString()));
		}
		else
		{
			return FLinearColor::White;
		}
	}

	FText GetHeight() const
	{
		FPakTextureEntryPtr PakFileItemPin = WeakPakTextureItem.Pin();
		if (PakFileItemPin.IsValid())
		{
			return FText::AsNumber(PakFileItemPin->Height);
		}
		else
		{
			return FText();
		}
	}

	FText GetSize() const
	{
		FPakTextureEntryPtr PakFileItemPin = WeakPakTextureItem.Pin();
		if (PakFileItemPin.IsValid())
		{
			return FText::AsMemory(PakFileItemPin->PakEntry.UncompressedSize, EMemoryUnitStandard::IEC);
		}
		else
		{
			return FText();
		}
	}

	FText GetSizeToolTip() const
	{
		FPakTextureEntryPtr PakFileItemPin = WeakPakTextureItem.Pin();
		if (PakFileItemPin.IsValid())
		{
			return FText::AsNumber(PakFileItemPin->PakEntry.UncompressedSize);
		}
		else
		{
			return FText();
		}
	}

	FText GetMipGen() const
	{
		FPakTextureEntryPtr PakFileItemPin = WeakPakTextureItem.Pin();
		if (PakFileItemPin.IsValid())
		{
			return FText::AsNumber(PakFileItemPin->MipGen);
		}
		else
		{
			return FText();
		}
	}

	FText GetLodBias() const
	{
		FPakTextureEntryPtr PakFileItemPin = WeakPakTextureItem.Pin();
		if (PakFileItemPin.IsValid())
		{
			return FText::AsNumber(PakFileItemPin->LodBias);
		}
		else
		{
			return FText();
		}
	}

	FText GetMaxSize() const
	{
		FPakTextureEntryPtr PakFileItemPin = WeakPakTextureItem.Pin();
		if (PakFileItemPin.IsValid())
		{
			return FText::AsNumber(PakFileItemPin->MaxSize);
		}
		else
		{
			return FText();
		}
	}

	FText GetTextureGroup() const
	{
		FPakTextureEntryPtr PakFileItemPin = WeakPakTextureItem.Pin();
		if (PakFileItemPin.IsValid())
		{
			return FText::AsNumber(PakFileItemPin->TextureGroup);
		}
		else
		{
			return FText();
		}
	}

	FText GetStreaming() const
	{
		FPakTextureEntryPtr PakFileItemPin = WeakPakTextureItem.Pin();
		if (PakFileItemPin.IsValid())
		{
			return FText::AsNumber(PakFileItemPin->bStreaming);
		}
		else
		{
			return FText();
		}
	}

	FText GetsRGB() const
	{
		FPakTextureEntryPtr PakFileItemPin = WeakPakTextureItem.Pin();
		if (PakFileItemPin.IsValid())
		{
			return FText::AsNumber(PakFileItemPin->sRGB);
		}
		else
		{
			return FText();
		}
	}

	FText GetTextureCompression() const
	{
		FPakTextureEntryPtr PakFileItemPin = WeakPakTextureItem.Pin();
		if (PakFileItemPin.IsValid())
		{
			return FText::AsNumber(PakFileItemPin->TextureCompression);
		}

		return FText();
	}
protected:
	TWeakPtr<SPakTextureView> WeakPakFileView;
	TWeakPtr<FPakTextureEntry> WeakPakTextureItem;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// SPakTextureView
////////////////////////////////////////////////////////////////////////////////////////////////////

SPakTextureView::SPakTextureView()
{
	FWidgetDelegates::GetOnLoadAssetRegistryFinishedDelegate().AddRaw(this, &SPakTextureView::OnLoadAssetReigstryFinished);
	FPakAnalyzerDelegates::OnPakLoadFinish.AddRaw(this, &SPakTextureView::OnLoadPakFinished);
	FPakAnalyzerDelegates::OnAssetParseFinish.AddRaw(this, &SPakTextureView::OnParseAssetFinished);
}

SPakTextureView::~SPakTextureView()
{
	FWidgetDelegates::GetOnLoadAssetRegistryFinishedDelegate().RemoveAll(this);
	FPakAnalyzerDelegates::OnPakLoadFinish.RemoveAll(this);
	FPakAnalyzerDelegates::OnAssetParseFinish.RemoveAll(this);

	if (SortAndFilterTask.IsValid())
	{
		SortAndFilterTask->Cancel();
		SortAndFilterTask->EnsureCompletion();
	}
}

void SPakTextureView::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot().VAlign(VAlign_Center).AutoHeight()
		[
			SNew(SBorder)
			.Padding(2.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot().VAlign(VAlign_Center).Padding(2.0f).AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot().Padding(0.f, 0.f, 5.f, 0.f).VAlign(VAlign_Center)
					[
						SNew(SComboButton)
						.ForegroundColor(FLinearColor::White)
						.ContentPadding(0)
						.ToolTipText(LOCTEXT("PakFilterToolTip", "Filter files by pak."))
						.OnGetMenuContent(this, &SPakTextureView::OnBuildPakFilterMenu)
						.HasDownArrow(true)
						.ContentPadding(FMargin(1.0f, 1.0f, 1.0f, 0.0f))
						.ButtonContent()
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(STextBlock)
								.Text(LOCTEXT("PakFilter", "Pak Filter")).ColorAndOpacity(FLinearColor::Green).ShadowOffset(FVector2D(1.f, 1.f))
							]
						]
					]

					+ SHorizontalBox::Slot()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f)
						[
							SAssignNew(SearchBox, SSearchBox)
							.HintText(LOCTEXT("SearchBoxHint", "Search files"))
							.OnTextChanged(this, &SPakTextureView::OnSearchBoxTextChanged)
							.IsEnabled(this, &SPakTextureView::SearchBoxIsEnabled)
							.ToolTipText(LOCTEXT("FilterSearchHint", "Type here to search files"))
						]

						+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(this, &SPakTextureView::GetFileCount)
						]
					]
				]
			]

		]

		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SBorder)
			.Padding(0.f)
			[
				SAssignNew(FileListView, SListView<FPakTextureEntryPtr>)
				.ItemHeight(20.f)
				.SelectionMode(ESelectionMode::Multi)
				//.OnMouseButtonClick()
				//.OnSelectiongChanged()
				.ListItemsSource(&FileCache)
				.OnGenerateRow(this, &SPakTextureView::OnGenerateFileRow)
				.ConsumeMouseWheel(EConsumeMouseWheel::Always)
				.OnContextMenuOpening(this, &SPakTextureView::OnGenerateContextMenu)
				.HeaderRow
				(
					SAssignNew(FileListHeaderRow, SHeaderRow).Visibility(EVisibility::Visible)
				)
			]
		]
	];

	InitializeAndShowHeaderColumns();

	SortAndFilterTask = MakeUnique<FAsyncTask<FTextureSortAndFilterTask>>(CurrentSortedColumn, CurrentSortMode, SharedThis(this));
	InnderTask = SortAndFilterTask.IsValid() ? &SortAndFilterTask->GetTask() : nullptr;

	check(SortAndFilterTask.IsValid());
	check(InnderTask);

	InnderTask->GetOnSortAndFilterFinishedDelegate().BindRaw(this, &SPakTextureView::OnSortAndFilterFinihed);

	FilesSummary = MakeShared<FPakTextureEntry>(TEXT("Total"), TEXT("Total"));
}

void SPakTextureView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	IPakAnalyzer* PakAnalyzer = IPakAnalyzerModule::Get().GetPakAnalyzer();
	if (PakAnalyzer)
	{
		if (bIsDirty)
		{
			if (SortAndFilterTask->IsDone())
			{
				MarkDirty(false);

				TMap<int32, bool> IndexFilterMap;
				for (int32 i = 0; i < PakFilterMap.Num(); ++i)
				{
					IndexFilterMap.Add(i, PakFilterMap[i].bShow);
				}

				InnderTask->SetWorkInfo(CurrentSortedColumn, CurrentSortMode, CurrentSearchText, IndexFilterMap);
				SortAndFilterTask->StartBackgroundTask();
			}
		}

		if (!DelayHighlightItem.IsEmpty() && !IsFileListEmpty())
		{
			ScrollToItem(DelayHighlightItem, DelayHighlightItemPakIndex);
			DelayHighlightItem = TEXT("");
			DelayHighlightItemPakIndex = -1;
		}
	}

	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

bool SPakTextureView::SearchBoxIsEnabled() const
{
	return true;
}

void SPakTextureView::OnSearchBoxTextChanged(const FText& InFilterText)
{
	if (CurrentSearchText.Equals(InFilterText.ToString(), ESearchCase::IgnoreCase))
	{
		return;
	}

	CurrentSearchText = InFilterText.ToString();
	MarkDirty(true);
}

TSharedRef<ITableRow> SPakTextureView::OnGenerateFileRow(FPakTextureEntryPtr InPakFileItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SPakTextureRow, InPakFileItem, SharedThis(this), OwnerTable);
}

TSharedPtr<SWidget> SPakTextureView::OnGenerateContextMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	// Extract menu
	MenuBuilder.BeginSection("Extract", LOCTEXT("ContextMenu_Header_Extract", "Extract"));
	{
		MenuBuilder.AddMenuEntry
		(
			LOCTEXT("ContextMenu_Extract", "Extract..."),
			LOCTEXT("ContextMenu_Extract_Desc", "Extract selected files to disk"),
			FSlateIcon(FUnrealPakViewerStyle::GetStyleSetName(), "Extract"),
			FUIAction
			(
				FExecuteAction::CreateSP(this, &SPakTextureView::OnExtract),
				FCanExecuteAction::CreateSP(this, &SPakTextureView::HasFileSelected)
			),
			NAME_None, EUserInterfaceActionType::Button
		);
	}
	MenuBuilder.EndSection();

	// Export menu
	MenuBuilder.BeginSection("Export", LOCTEXT("ContextMenu_Header_Export", "Export File Info"));
	{
		MenuBuilder.AddMenuEntry
		(
			LOCTEXT("ContextMenu_Export_To_Json", "Export To Json..."),
			LOCTEXT("ContextMenu_Export_To_Json_Desc", "Export selected file(s) info to json"),
			FSlateIcon(FUnrealPakViewerStyle::GetStyleSetName(), "Export"),
			FUIAction
			(
				FExecuteAction::CreateSP(this, &SPakTextureView::OnExportToJson),
				FCanExecuteAction::CreateSP(this, &SPakTextureView::HasFileSelected)
			),
			NAME_None, EUserInterfaceActionType::Button
		);

		MenuBuilder.AddMenuEntry
		(
			LOCTEXT("ContextMenu_Export_To_Csv", "Export To Csv..."),
			LOCTEXT("ContextMenu_Export_To_Csv_Desc", "Export selected file(s) info to csv"),
			FSlateIcon(FUnrealPakViewerStyle::GetStyleSetName(), "Export"),
			FUIAction
			(
				FExecuteAction::CreateSP(this, &SPakTextureView::OnExportToCsv),
				FCanExecuteAction::CreateSP(this, &SPakTextureView::HasFileSelected)
			),
			NAME_None, EUserInterfaceActionType::Button
		);
	}
	MenuBuilder.EndSection();

	// Operation menu
	MenuBuilder.BeginSection("Operation", LOCTEXT("ContextMenu_Header_Operation", "Operation"));
	{
		FUIAction Action_JumpToTreeView
		(
			FExecuteAction::CreateSP(this, &SPakTextureView::OnJumpToTreeViewExecute),
			FCanExecuteAction::CreateSP(this, &SPakTextureView::HasOneFileSelected)
		);
		MenuBuilder.AddMenuEntry
		(
			LOCTEXT("ContextMenu_Columns_JumpToTreeView", "Show In Tree View"),
			LOCTEXT("ContextMenu_Columns_JumpToTreeView_Desc", "Show current selected file in tree view"),
			FSlateIcon(FUnrealPakViewerStyle::GetStyleSetName(), "Find"), Action_JumpToTreeView, NAME_None, EUserInterfaceActionType::Button
		);

		MenuBuilder.AddSubMenu
		(
			LOCTEXT("ContextMenu_Header_Columns_Copy", "Copy Column(s)"),
			LOCTEXT("ContextMenu_Header_Columns_Copy_Desc", "Copy value of column(s)"),
			FNewMenuDelegate::CreateSP(this, &SPakTextureView::OnBuildCopyColumnMenu),
			false,
			FSlateIcon(FUnrealPakViewerStyle::GetStyleSetName(), "Copy")
		);
	}
	MenuBuilder.EndSection();

	// Columns menu
	MenuBuilder.BeginSection("Columns", LOCTEXT("ContextMenu_Header_Columns", "Columns"));
	{
		MenuBuilder.AddSubMenu
		(
			LOCTEXT("ContextMenu_Header_Columns_View", "View Column"),
			LOCTEXT("ContextMenu_Header_Columns_View_Desc", "Hides or shows columns"),
			FNewMenuDelegate::CreateSP(this, &SPakTextureView::OnBuildViewColumnMenu),
			false,
			FSlateIcon(FUnrealPakViewerStyle::GetStyleSetName(), "View")
		);

		FUIAction Action_ShowAllColumns
		(
			FExecuteAction::CreateSP(this, &SPakTextureView::OnShowAllColumnsExecute),
			FCanExecuteAction::CreateSP(this, &SPakTextureView::OnShowAllColumnsCanExecute)
		);
		MenuBuilder.AddMenuEntry
		(
			LOCTEXT("ContextMenu_Header_Columns_ShowAllColumns", "Show All Columns"),
			LOCTEXT("ContextMenu_Header_Columns_ShowAllColumns_Desc", "Resets tree view to show all columns"),
			FSlateIcon(FUnrealPakViewerStyle::GetStyleSetName(), "View"), Action_ShowAllColumns, NAME_None, EUserInterfaceActionType::Button
		);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SPakTextureView::OnBuildSortByMenu(FMenuBuilder& MenuBuilder)
{

}

void SPakTextureView::OnBuildCopyColumnMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("CopyAllColumns", LOCTEXT("ContextMenu_Header_Columns_CopyAllColumns", "Copy All Columns"));
	{
		FUIAction Action_CopyAllColumns
		(
			FExecuteAction::CreateSP(this, &SPakTextureView::OnCopyAllColumnsExecute),
			FCanExecuteAction::CreateSP(this, &SPakTextureView::HasFileSelected)
		);
		MenuBuilder.AddMenuEntry
		(
			LOCTEXT("ContextMenu_Columns_CopyAllColumns", "Copy All Columns"),
			LOCTEXT("ContextMenu_Columns_CopyAllColumns_Desc", "Copy all columns of selected file to clipboard"),
			FSlateIcon(FUnrealPakViewerStyle::GetStyleSetName(), "Copy"), Action_CopyAllColumns, NAME_None, EUserInterfaceActionType::Button
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("CopyColumn", LOCTEXT("ContextMenu_Header_Columns_CopySingleColumn", "Copy Column"));
	{
		for (const auto& ColumnPair : TextureColumns)
		{
			const FTextureColumn& Column = ColumnPair.Value;

			if (Column.IsVisible())
			{
				FUIAction Action_CopyColumn
				(
					FExecuteAction::CreateSP(this, &SPakTextureView::OnCopyColumnExecute, Column.GetId()),
					FCanExecuteAction::CreateSP(this, &SPakTextureView::HasFileSelected)
				);

				MenuBuilder.AddMenuEntry
				(
					FText::Format(LOCTEXT("ContextMenu_Columns_CopySingleColumn", "Copy {0}(s)"), Column.GetTitleName()),
					FText::Format(LOCTEXT("ContextMenu_Columns_CopySingleColumn_Desc", "Copy {0}(s) of selected file to clipboard"), Column.GetTitleName()),
					FSlateIcon(FUnrealPakViewerStyle::GetStyleSetName(), "Copy"), Action_CopyColumn, NAME_None, EUserInterfaceActionType::Button
				);
			}
		}
	}
	MenuBuilder.EndSection();
}

void SPakTextureView::OnBuildViewColumnMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("ViewColumn", LOCTEXT("ContextMenu_Header_Columns_View", "View Column"));

	for (const auto& ColumnPair : TextureColumns)
	{
		const FTextureColumn& Column = ColumnPair.Value;

		FUIAction Action_ToggleColumn
		(
			FExecuteAction::CreateSP(this, &SPakTextureView::ToggleColumnVisibility, Column.GetId()),
			FCanExecuteAction::CreateSP(this, &SPakTextureView::CanToggleColumnVisibility, Column.GetId()),
			FIsActionChecked::CreateSP(this, &SPakTextureView::IsColumnVisible, Column.GetId())
		);
		MenuBuilder.AddMenuEntry
		(
			Column.GetTitleName(),
			Column.GetDescription(),
			FSlateIcon(), Action_ToggleColumn, NAME_None, EUserInterfaceActionType::ToggleButton
		);
	}

	MenuBuilder.EndSection();
}

TSharedRef<SWidget> SPakTextureView::OnBuildPakFilterMenu()
{
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, nullptr);

	IPakAnalyzer* PakAnalyzer = IPakAnalyzerModule::Get().GetPakAnalyzer();
	if (PakAnalyzer)
	{
		if (PakFilterMap.Num() > 0)
		{
			MenuBuilder.BeginSection("FileViewShowAllPaks", LOCTEXT("QuickFilterHeading", "Quick Filter"));
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("ShowAllPaks", "Show/Hide All"),
					LOCTEXT("ShowAllPaks_Tooltip", "Change filtering to show/hide all paks"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &SPakTextureView::OnShowAllPaksExecute),
						FCanExecuteAction(),
						FIsActionChecked::CreateSP(this, &SPakTextureView::IsShowAllPaksChecked)),
					NAME_None,
					EUserInterfaceActionType::ToggleButton
				);
			}
			MenuBuilder.EndSection();
		}

		MenuBuilder.BeginSection("FileViewPaksEntries", LOCTEXT("PaksHeading", "Paks"));
		for (int32 i = 0; i < PakFilterMap.Num(); ++i)
		{
			const FPakFilterInfo& Info = PakFilterMap[i];
			const FString PakString = Info.PakName.ToString();
			const FText PakText(FText::AsCultureInvariant(PakString));

			const TSharedRef<SWidget> TextBlock = SNew(STextBlock)
				.Text(PakText)
				.ShadowColorAndOpacity(FLinearColor(0.05f, 0.05f, 0.05f, 1.0f))
				.ShadowOffset(FVector2D(1.0f, 1.0f))
				.ColorAndOpacity(FSlateColor(FClassColumn::GetColorByClass(*PakString)));

			MenuBuilder.AddMenuEntry(
				FUIAction(FExecuteAction::CreateSP(this, &SPakTextureView::OnTogglePakExecute, i),
					FCanExecuteAction(),
					FIsActionChecked::CreateSP(this, &SPakTextureView::IsPakFilterChecked, i)),
				TextBlock,
				NAME_None,
				FText::Format(LOCTEXT("Pak_Tooltip", "Filter the File View to show/hide pak: {0}"), PakText),
				EUserInterfaceActionType::ToggleButton
			);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

void SPakTextureView::OnShowAllPaksExecute()
{
	const bool bIsShowAll = IsShowAllPaksChecked();
	for (FPakFilterInfo& Info : PakFilterMap)
	{
		Info.bShow = !bIsShowAll;
	}

	MarkDirty(true);
}

bool SPakTextureView::IsShowAllPaksChecked() const
{
	for (const FPakFilterInfo& Info : PakFilterMap)
	{
		if (!Info.bShow)
		{
			return false;
		}
	}

	return true;
}

void SPakTextureView::OnTogglePakExecute(int32 InPakIndex)
{
	if (PakFilterMap.IsValidIndex(InPakIndex))
	{
		PakFilterMap[InPakIndex].bShow = !PakFilterMap[InPakIndex].bShow;
		MarkDirty(true);
	}
}

bool SPakTextureView::IsPakFilterChecked(int32 InPakIndex) const
{
	if (PakFilterMap.IsValidIndex(InPakIndex))
	{
		return PakFilterMap[InPakIndex].bShow;
	}

	return false;
}

void SPakTextureView::FillPaksFilter()
{
	PakFilterMap.Empty();

	IPakAnalyzer* PakAnalyzer = IPakAnalyzerModule::Get().GetPakAnalyzer();
	if (PakAnalyzer)
	{
		const TArray<FPakFileSumaryPtr>& Summaries = PakAnalyzer->GetPakFileSumary();
		PakFilterMap.AddZeroed(Summaries.Num());

		for (int32 i = 0; i < Summaries.Num(); ++i)
		{
			FPakFilterInfo Info;
			Info.bShow = true;
			Info.PakName = *FPaths::GetCleanFilename(Summaries[i]->PakFilePath);

			PakFilterMap[i] = Info;
		}
	}
}

void SPakTextureView::InitializeAndShowHeaderColumns()
{
	TextureColumns.Empty();

	// Name Column
	FTextureColumn& NameColumn = TextureColumns.Emplace(FTextureColumn::NameColumnName, 
		FTextureColumn(0, FTextureColumn::NameColumnName,
			LOCTEXT("NameColumn", "Name"), LOCTEXT("NameColumnTip", "File name"), 2.f,
		EFileColumnFlags::ShouldBeVisible | EFileColumnFlags::CanBeFiltered)
	);
	NameColumn.SetAscendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->Filename.LexicalLess(B->Filename);
		}
	);
	NameColumn.SetDescendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return B->Filename.LexicalLess(A->Filename);
		}
	);

	// Path Column
	FTextureColumn& PathColumn = TextureColumns.Emplace(FTextureColumn::PathColumnName,
		FTextureColumn(1, FTextureColumn::PathColumnName, 
			LOCTEXT("PathColumn", "Path"), LOCTEXT("PathColumnTip", "File path in pak"), 3.f, 
			EFileColumnFlags::ShouldBeVisible | EFileColumnFlags::CanBeFiltered | EFileColumnFlags::CanBeHidden)
	);
	PathColumn.SetAscendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->Path < B->Path;
		}
	);
	PathColumn.SetDescendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return B->Path < A->Path;
		}
	);

	// Size Column
	FTextureColumn& SizeColumn = TextureColumns.Emplace(FTextureColumn::SizeColumnName,
		FTextureColumn(2, FTextureColumn::SizeColumnName, 
			LOCTEXT("SizeColumn", "Size"), LOCTEXT("SizeColumnTip", "File original size"), 1.f, 
			EFileColumnFlags::ShouldBeVisible | EFileColumnFlags::CanBeFiltered | EFileColumnFlags::CanBeHidden)
	);
	SizeColumn.SetAscendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->PakEntry.UncompressedSize < B->PakEntry.UncompressedSize;
		}
	);
	SizeColumn.SetDescendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return B->PakEntry.UncompressedSize < A->PakEntry.UncompressedSize;
		}
	);

	// Width Column
	FTextureColumn& WidthColumn = TextureColumns.Emplace(FTextureColumn::WidthColumnName, 
		FTextureColumn(3, FTextureColumn::WidthColumnName, 
			LOCTEXT("WidthColumn", "Width"), LOCTEXT("WidthColumnTip", "Texture Width"), 1.f, 
			EFileColumnFlags::ShouldBeVisible | EFileColumnFlags::CanBeFiltered | EFileColumnFlags::CanBeHidden)
	);
	WidthColumn.SetAscendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			if (A->Width == B->Width)
			{
				return A->Height > B->Height;
			}
			else
			{
				return A->Width > B->Width;
			}
		}
	);
	WidthColumn.SetDescendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			if (A->Width == B->Width)
			{
				return A->Height < B->Height;
			}
			else
			{
				return A->Width < B->Width;
			}
		}
	);

	// Height Column
	FTextureColumn& HeightColumn = TextureColumns.Emplace(FTextureColumn::HeightColumnName,
		FTextureColumn(4, FTextureColumn::HeightColumnName, 
			LOCTEXT("HeightColumn", "Height"), LOCTEXT("HeightColumnTip", "Texture Height"), 1.f, 
			EFileColumnFlags::ShouldBeVisible | EFileColumnFlags::CanBeFiltered | EFileColumnFlags::CanBeHidden)
	);
	HeightColumn.SetAscendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			
			if (A->Height == B->Height)
			{
				return A->Width < B->Width;
			}
			else
			{
				return A->Height < B->Height;
			}
		}
	);
	HeightColumn.SetDescendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			if (A->Height == B->Height)
			{
				return A->Width > B->Width;
			}
			else
			{
				return A->Height > B->Height;
			}
			
		}
	);

	// MipGen Column
	FTextureColumn& MipGenColumn = TextureColumns.Emplace(FTextureColumn::MipGenSettingColumnName, 
		FTextureColumn(5, FTextureColumn::MipGenSettingColumnName, 
			LOCTEXT("MipGenColumn", "MipGen"), LOCTEXT("MipGenColumnTip", "MipGen Setting"), 1.f, 
			EFileColumnFlags::ShouldBeVisible | EFileColumnFlags::CanBeFiltered | EFileColumnFlags::CanBeHidden)
	);
	MipGenColumn.SetAscendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->MipGen < B->MipGen;
		}
	);
	MipGenColumn.SetDescendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->MipGen > B->MipGen;
		}
	);
	
	// LodBias Column
	FTextureColumn& LodBiasColumn = TextureColumns.Emplace(FTextureColumn::LodBiasColumnName, 
		FTextureColumn(6, FTextureColumn::LodBiasColumnName, 
			LOCTEXT("LodBiasColumn", "LodBias"), LOCTEXT("LodBiasColumnTip", "Texture LodBias"), 0.5f, 
			EFileColumnFlags::ShouldBeVisible | EFileColumnFlags::CanBeFiltered | EFileColumnFlags::CanBeHidden));
	LodBiasColumn.SetAscendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->LodBias < B->LodBias;
		}
	);
	LodBiasColumn.SetAscendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->LodBias > B->LodBias;
		}
	);
	
	// MaxSize
	FTextureColumn& MaxSizeColumn = TextureColumns.Emplace(FTextureColumn::MaxSizeColumnName, 
		FTextureColumn(7, FTextureColumn::MaxSizeColumnName, 
			LOCTEXT("MaxSizeColumn", "MaxSize"), LOCTEXT("MaxSizeColumnTip", "TextureMaxSize"), 1.f, 
			EFileColumnFlags::ShouldBeVisible | EFileColumnFlags::CanBeFiltered | EFileColumnFlags::CanBeHidden));
	MaxSizeColumn.SetAscendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->MaxSize < B->MaxSize;
		}
	);
	MaxSizeColumn.SetDescendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->MaxSize > B->MaxSize;
		}
	);
	
	// TextureGroup
	FTextureColumn& GroupColumn = TextureColumns.Emplace(FTextureColumn::GroupColumnName, 
		FTextureColumn(8, FTextureColumn::GroupColumnName,
			LOCTEXT("TextureGroupColumn", "TextureGroup"), LOCTEXT("TextureGroupColumnTip", "TextureGroup"), 1.f, 
			EFileColumnFlags::ShouldBeVisible | EFileColumnFlags::CanBeHidden));
	GroupColumn.SetAscendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->TextureGroup < B->TextureGroup;
		}
	);
	GroupColumn.SetDescendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->TextureGroup > B->TextureGroup;
		}
	);

	// Streaming
	FTextureColumn& StreamingColumn = TextureColumns.Emplace(FTextureColumn::IsStreamingColumnName,
		FTextureColumn(10, FTextureColumn::IsStreamingColumnName,
			LOCTEXT("Streaming", "Streaming"), LOCTEXT("StreamingTip", "Texture Streaming"), 0.5f, 
			EFileColumnFlags::ShouldBeVisible | EFileColumnFlags::CanBeHidden));
	StreamingColumn.SetAscendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->bStreaming < B->bStreaming;
		}
	);
	StreamingColumn.SetDescendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->bStreaming > B->bStreaming;
		}
	);
	
	// sRGB
	FTextureColumn& sRGBColumn = TextureColumns.Emplace(FTextureColumn::IsSRGBColumnName, 
		FTextureColumn(9, FTextureColumn::IsSRGBColumnName, 
			LOCTEXT("sRGB", "sRGB"), LOCTEXT("sRGBColumnColumnTip", "Is sRGB"), 0.5f, 
			EFileColumnFlags::ShouldBeVisible | EFileColumnFlags::CanBeHidden | EFileColumnFlags::CanBeFiltered));
	sRGBColumn.SetAscendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->sRGB < B->sRGB;
		}
	);
	sRGBColumn.SetDescendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->sRGB > B->sRGB;
		}
	);

	// Texture Compression
	FTextureColumn& TCColumn = TextureColumns.Emplace(FTextureColumn::CompressionColumnName, 
		FTextureColumn(10, FTextureColumn::CompressionColumnName,
			LOCTEXT("TextureCompression", "Compression"), LOCTEXT("TextureCompressionColumnTip", "TextureCompression"), 1.f,
			EFileColumnFlags::ShouldBeVisible | EFileColumnFlags::CanBeHidden));
	TCColumn.SetAscendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->TextureCompression < B->TextureCompression;
		}
	);
	TCColumn.SetDescendingCompareDelegate(
		[](const FPakTextureEntryPtr& A, const FPakTextureEntryPtr& B) -> bool
		{
			return A->TextureCompression < B->TextureCompression;
		}
	);

	// Show columns.
	for (const auto& ColumnPair : TextureColumns)
	{
		if (ColumnPair.Value.ShouldBeVisible())
		{
			ShowColumn(ColumnPair.Key);
		}
	}
}

const FTextureColumn* SPakTextureView::FindCoulum(const FName ColumnId) const
{
	return TextureColumns.Find(ColumnId);
}

FTextureColumn* SPakTextureView::FindCoulum(const FName ColumnId)
{
	return TextureColumns.Find(ColumnId);
}

FText SPakTextureView::GetSearchText() const
{
	return SearchBox.IsValid() ? SearchBox->GetText() : FText();
}

EColumnSortMode::Type SPakTextureView::GetSortModeForColumn(const FName ColumnId) const
{
	if (CurrentSortedColumn != ColumnId)
	{
		return EColumnSortMode::None;
	}

	return CurrentSortMode;
}

void SPakTextureView::OnSortModeChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type SortMode)
{
	FTextureColumn* Column = FindCoulum(ColumnId);
	if (!Column)
	{
		return;
	}

	if (!Column->CanBeSorted())
	{
		return;
	}

	CurrentSortedColumn = ColumnId;
	CurrentSortMode = SortMode;
	MarkDirty(true);
}

bool SPakTextureView::CanShowColumn(const FName ColumnId) const
{
	return true;
}

void SPakTextureView::ShowColumn(const FName ColumnId)
{
	FTextureColumn* Column = FindCoulum(ColumnId);
	if (!Column)
	{
		return;
	}

	Column->Show();

	SHeaderRow::FColumn::FArguments ColumnArgs;
	ColumnArgs
		.ColumnId(Column->GetId())
		.DefaultLabel(Column->GetTitleName())
		.HAlignHeader(HAlign_Fill)
		.VAlignHeader(VAlign_Fill)
		.HeaderContentPadding(FMargin(2.f))
		.HAlignCell(HAlign_Fill)
		.VAlignCell(VAlign_Fill)
		.SortMode(this, &SPakTextureView::GetSortModeForColumn, Column->GetId())
		.OnSort(this, &SPakTextureView::OnSortModeChanged)
		//.ManualWidth(Column->GetInitialWidth())
		.FillWidth(Column->GetFillWidth())
		.HeaderContent()
		[
			SNew(SBox)
			.ToolTipText(Column->GetDescription())
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Column->GetTitleName())
			]
		];

	int32 ColumnIndex = 0;
	const int32 TargetColumnIndex = Column->GetIndex();
	const TIndirectArray<SHeaderRow::FColumn>& Columns = FileListHeaderRow->GetColumns();
	const int32 CurrentColumnCount = Columns.Num();
	for (; ColumnIndex < CurrentColumnCount; ++ColumnIndex)
	{
		const SHeaderRow::FColumn& CurrentColumn = Columns[ColumnIndex];
		FTextureColumn* FileColumn = FindCoulum(CurrentColumn.ColumnId);
		if (TargetColumnIndex < FileColumn->GetIndex())
		{
			break;
		}
	}

	FileListHeaderRow->InsertColumn(ColumnArgs, ColumnIndex);
}

bool SPakTextureView::CanHideColumn(const FName ColumnId)
{
	FTextureColumn* Column = FindCoulum(ColumnId);

	return Column ? Column->CanBeHidden() : false;
}

void SPakTextureView::HideColumn(const FName ColumnId)
{
	FTextureColumn* Column = FindCoulum(ColumnId);
	if (Column)
	{
		Column->Hide();
		FileListHeaderRow->RemoveColumn(ColumnId);
	}
}

bool SPakTextureView::IsColumnVisible(const FName ColumnId)
{
	const FTextureColumn* Column = FindCoulum(ColumnId);

	return Column ? Column->IsVisible() : false;
}

bool SPakTextureView::CanToggleColumnVisibility(const FName ColumnId)
{
	FTextureColumn* Column = FindCoulum(ColumnId);

	return Column ? !Column->IsVisible() || Column->CanBeHidden() : false;
}

void SPakTextureView::ToggleColumnVisibility(const FName ColumnId)
{
	FTextureColumn* Column = FindCoulum(ColumnId);
	if (!Column)
	{
		return;
	}

	if (Column->IsVisible())
	{
		HideColumn(ColumnId);
	}
	else
	{
		ShowColumn(ColumnId);
	}
}

bool SPakTextureView::OnShowAllColumnsCanExecute() const
{
	return true;
}

void SPakTextureView::OnShowAllColumnsExecute()
{
	// Show columns.
	for (const auto& ColumnPair : TextureColumns)
	{
		if (!ColumnPair.Value.IsVisible())
		{
			ShowColumn(ColumnPair.Key);
		}
	}
}

bool SPakTextureView::HasOneFileSelected() const
{
	TArray<FPakTextureEntryPtr> SelectedItems;
	GetSelectedItems(SelectedItems);

	return SelectedItems.Num() == 1;
}

bool SPakTextureView::HasFileSelected() const
{
	TArray<FPakTextureEntryPtr> SelectedItems;
	GetSelectedItems(SelectedItems);

	return SelectedItems.Num() > 0;
}

void SPakTextureView::OnCopyAllColumnsExecute()
{
	FString Value;

	IPakAnalyzer* PakAnalyzer = IPakAnalyzerModule::Get().GetPakAnalyzer();

	TArray<FPakTextureEntryPtr> SelectedItems;
	GetSelectedItems(SelectedItems);

	if (SelectedItems.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> FileObjects;

		for (const FPakTextureEntryPtr PakFileItem : SelectedItems)
		{
			if (PakFileItem.IsValid())
			{
				TSharedRef<FJsonObject> FileObject = MakeShareable(new FJsonObject);

				FileObject->SetStringField(TEXT("Name"), PakFileItem->Filename.ToString());
				FileObject->SetStringField(TEXT("Path"), PakFileItem->Path);
				FileObject->SetNumberField(TEXT("Size"), PakFileItem->PakEntry.UncompressedSize);
				FileObject->SetNumberField(TEXT("Width"), PakFileItem->Width);
				FileObject->SetNumberField(TEXT("Height"), PakFileItem->Height);
				FileObject->SetNumberField(TEXT("MipGen"), PakFileItem->MipGen);
				FileObject->SetNumberField(TEXT("LodBias"), PakFileItem->LodBias);
				FileObject->SetNumberField(TEXT("MaxSize"), PakFileItem->MaxSize);
				FileObject->SetNumberField(TEXT("TextureGroup"), PakFileItem->TextureGroup);
				FileObject->SetNumberField(TEXT("Streaming"), PakFileItem->bStreaming);
				FileObject->SetNumberField(TEXT("sRGB"), PakFileItem->sRGB);
				FileObject->SetNumberField(TEXT("TextureCompression"), PakFileItem->TextureCompression);

				FileObjects.Add(MakeShareable(new FJsonValueObject(FileObject)));
			}
		}

		TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&Value);
		FJsonSerializer::Serialize(FileObjects, JsonWriter);
		JsonWriter->Close();
	}

	FPlatformApplicationMisc::ClipboardCopy(*Value);
}

void SPakTextureView::OnCopyColumnExecute(const FName ColumnId)
{
	IPakAnalyzer* PakAnalyzer = IPakAnalyzerModule::Get().GetPakAnalyzer();

	TArray<FString> Values;
	TArray<FPakTextureEntryPtr> SelectedItems;
	GetSelectedItems(SelectedItems);

	for (const FPakTextureEntryPtr PakFileItem : SelectedItems)
	{
		if (PakFileItem.IsValid())
		{
			const FPakEntry* PakEntry = &PakFileItem->PakEntry;

			if (ColumnId == FTextureColumn::NameColumnName)
			{
				Values.Add(PakFileItem->Filename.ToString());
			}
			else if (ColumnId == FTextureColumn::PathColumnName)
			{
				Values.Add(PakFileItem->Path);
			}
			else if (ColumnId == FTextureColumn::SizeColumnName)
			{
				Values.Add(FString::Printf(TEXT("%lld"), PakEntry->UncompressedSize));
			}
			else if (ColumnId == FTextureColumn::WidthColumnName)
			{
				Values.Add(FString::Printf(TEXT("%d"), PakFileItem->Width));
			}
			else if (ColumnId == FTextureColumn::HeightColumnName)
			{
				Values.Add(FString::Printf(TEXT("%d"), PakFileItem->Height));
			}
			else if (ColumnId == FTextureColumn::MipGenSettingColumnName)
			{
				Values.Add(FString::Printf(TEXT("%d"), PakFileItem->MipGen));
			}
			else if (ColumnId == FTextureColumn::LodBiasColumnName)
			{
				Values.Add(FString::Printf(TEXT("%d"), PakFileItem->LodBias));
			}
			else if (ColumnId == FTextureColumn::MaxSizeColumnName)
			{
				Values.Add(FString::Printf(TEXT("%d"), PakFileItem->MaxSize));
			}
			else if (ColumnId == FTextureColumn::GroupColumnName)
			{
				Values.Add(FString::Printf(TEXT("%d"), PakFileItem->TextureGroup));
			}
			else if (ColumnId == FTextureColumn::IsStreamingColumnName)
			{
				Values.Add(FString::Printf(TEXT("%d"), PakFileItem->TextureGroup));
			}
			else if (ColumnId == FTextureColumn::IsSRGBColumnName)
			{
				Values.Add(FString::Printf(TEXT("%d"), PakFileItem->sRGB));
			}
			else if (ColumnId == FTextureColumn::CompressionColumnName)
			{
				Values.Add(FString::Printf(TEXT("%d"), PakFileItem->TextureCompression));
			}
		}
	}

	FPlatformApplicationMisc::ClipboardCopy(*FString::Join(Values, TEXT("\n")));
}

void SPakTextureView::OnJumpToTreeViewExecute()
{
	TArray<FPakTextureEntryPtr> SelectedItems;
	GetSelectedItems(SelectedItems);

	if (SelectedItems.Num() > 0 && SelectedItems[0].IsValid())
	{
		FWidgetDelegates::GetOnSwitchToTreeViewDelegate().Broadcast(SelectedItems[0]->Path, SelectedItems[0]->OwnerPakIndex);
	}
}

void SPakTextureView::MarkDirty(bool bInIsDirty)
{
	bIsDirty = bInIsDirty;
}

void SPakTextureView::OnSortAndFilterFinihed(const FName InSortedColumn, EColumnSortMode::Type InSortMode, const FString& InSearchText)
{
	FFunctionGraphTask::CreateAndDispatchWhenReady([this, InSearchText]()
		{
			IPakAnalyzer* PakAnalyzer = IPakAnalyzerModule::Get().GetPakAnalyzer();

			InnderTask->RetriveResult(FileCache);
			FillFilesSummary();

			FileListView->RebuildList();

			if (!InSearchText.Equals(CurrentSearchText, ESearchCase::IgnoreCase))
			{
				// Search text changed during sort or filter
				MarkDirty(true);
			}
		},
		TStatId(), nullptr, ENamedThreads::GameThread);
}

FText SPakTextureView::GetFileCount() const
{
	IPakAnalyzer* Analyzer = IPakAnalyzerModule::Get().GetPakAnalyzer();
	const int32 CurrentFileCount = FMath::Clamp(FileCache.Num() - 1, 0, FileCache.Num());

	int32 TotalFileCount = 0;
	if (Analyzer)
	{
		const TArray<FPakFileSumaryPtr>& Summaries = Analyzer->GetPakFileSumary();
		for (const FPakFileSumaryPtr& Summary : Summaries)
		{
			TotalFileCount += Summary->FileCount;
		}
	}

	return FText::Format(FTextFormat::FromString(TEXT("{0} / {1} files")), FText::AsNumber(CurrentFileCount), FText::AsNumber(TotalFileCount));
}

bool SPakTextureView::IsFileListEmpty() const
{
	return FileCache.Num() - 1 <= 0;
}

void SPakTextureView::OnExportToJson()
{
	bool bOpened = false;
	TArray<FString> OutFileNames;

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		FSlateApplication::Get().CloseToolTip();

		bOpened = DesktopPlatform->SaveFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			LOCTEXT("OpenExportDialogTitleText", "Select output json file path...").ToString(),
			TEXT(""),
			TEXT(""),
			TEXT("Json Files (*.json)|*.json|All Files (*.*)|*.*"),
			EFileDialogFlags::None,
			OutFileNames);
	}

	if (!bOpened || OutFileNames.Num() <= 0)
	{
		return;
	}

	TArray<FPakTextureEntryPtr> SelectedItems;
	GetSelectedItems(SelectedItems);

	//IPakAnalyzerModule::Get().GetPakAnalyzer()->ExportToJson(OutFileNames[0], SelectedItems);
}

void SPakTextureView::OnExportToCsv()
{
	bool bOpened = false;
	TArray<FString> OutFileNames;

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		FSlateApplication::Get().CloseToolTip();

		bOpened = DesktopPlatform->SaveFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			LOCTEXT("OpenExportDialogTitleText", "Select output csv file path...").ToString(),
			TEXT(""),
			TEXT(""),
			TEXT("Json Files (*.csv)|*.csv|All Files (*.*)|*.*"),
			EFileDialogFlags::None,
			OutFileNames);
	}

	if (!bOpened || OutFileNames.Num() <= 0)
	{
		return;
	}

	TArray<FPakTextureEntryPtr> SelectedItems;
	GetSelectedItems(SelectedItems);

	//IPakAnalyzerModule::Get().GetPakAnalyzer()->ExportToCsv(OutFileNames[0], SelectedItems);
}

void SPakTextureView::OnExtract()
{
	bool bOpened = false;
	FString OutputPath;

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		FSlateApplication::Get().CloseToolTip();

		bOpened = DesktopPlatform->OpenDirectoryDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			LOCTEXT("OpenExtractDialogTitleText", "Select output path...").ToString(),
			TEXT(""),
			OutputPath);
	}

	if (!bOpened)
	{
		return;
	}

	TArray<FPakTextureEntryPtr> SelectedItems;
	GetSelectedItems(SelectedItems);

	//IPakAnalyzerModule::Get().GetPakAnalyzer()->ExtractFiles(OutputPath, SelectedItems);
}

void SPakTextureView::ScrollToItem(const FString& InPath, int32 PakIndex)
{
	for (const FPakTextureEntryPtr FileEntry : FileCache)
	{
		if (FileEntry->Path.Equals(InPath, ESearchCase::IgnoreCase) && FileEntry->OwnerPakIndex == PakIndex)
		{
			TArray<FPakTextureEntryPtr> SelectArray = { FileEntry };
			FileListView->SetItemSelection(SelectArray, true, ESelectInfo::Direct);
			FileListView->RequestScrollIntoView(FileEntry);
			return;
		}
	}
}

void SPakTextureView::OnLoadAssetReigstryFinished()
{
	MarkDirty(true);
}

void SPakTextureView::OnLoadPakFinished()
{
	FillPaksFilter();
	MarkDirty(true);
}

void SPakTextureView::OnParseAssetFinished()
{
	MarkDirty(true);
}

void SPakTextureView::FillFilesSummary()
{
	FilesSummary->PakEntry.Offset = 0;
	FilesSummary->PakEntry.UncompressedSize = 0;
	FilesSummary->PakEntry.Size = 0;

	if (FileCache.Num() > 0)
	{
		for (FPakTextureEntryPtr PakFileEntryPtr : FileCache)
		{
			FilesSummary->PakEntry.UncompressedSize += PakFileEntryPtr->PakEntry.UncompressedSize;
			FilesSummary->PakEntry.Size += PakFileEntryPtr->PakEntry.Size;
		}

		FileCache.Add(FilesSummary);
	}
}

bool SPakTextureView::GetSelectedItems(TArray<FPakTextureEntryPtr>& OutSelectedItems) const
{
	if (FileListView.IsValid())
	{
		FileListView->GetSelectedItems(OutSelectedItems);
		OutSelectedItems.Remove(FilesSummary);
		return true;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
