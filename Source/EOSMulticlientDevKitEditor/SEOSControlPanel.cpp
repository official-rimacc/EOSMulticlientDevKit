// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Control Panel Root Widget

#include "SEOSControlPanel.h"
#include "SEOSSetupTab.h"
#include "SEOSValidateTab.h"
#include "SEOSQuickLaunchTab.h"
#include "SEOSPackageTab.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "EOSMulticlientDevKitEditor"

void SEOSControlPanel::Construct(const FArguments& InArgs)
{
	auto MakeTabButton = [this](int32 Index, const FText& Label) -> TSharedRef<SWidget>
	{
		return SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.OnClicked(this, &SEOSControlPanel::OnTabClicked, Index)
			.ContentPadding(FMargin(16, 8))
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(this, &SEOSControlPanel::GetTabButtonFont, Index)
				.ColorAndOpacity(this, &SEOSControlPanel::GetTabButtonColor, Index)
			];
	};

	ChildSlot
	[
		SNew(SVerticalBox)

		// Tab bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8, 8, 8, 0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth() [ MakeTabButton(0, LOCTEXT("SetupTab", "Setup")) ]
			+ SHorizontalBox::Slot().AutoWidth() [ MakeTabButton(1, LOCTEXT("ValidateTab", "Validate")) ]
			+ SHorizontalBox::Slot().AutoWidth() [ MakeTabButton(2, LOCTEXT("QuickLaunchTab", "Quick Launch")) ]
			+ SHorizontalBox::Slot().AutoWidth() [ MakeTabButton(3, LOCTEXT("PackageTab", "Package")) ]
		]

		// Separator
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8, 0)
		[
			SNew(SSeparator)
		]

		// Tab content
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex(this, &SEOSControlPanel::GetActiveTabIndex)
			+ SWidgetSwitcher::Slot() [ SNew(SEOSSetupTab) ]
			+ SWidgetSwitcher::Slot() [ SNew(SEOSValidateTab) ]
			+ SWidgetSwitcher::Slot() [ SNew(SEOSQuickLaunchTab) ]
			+ SWidgetSwitcher::Slot() [ SNew(SEOSPackageTab) ]
		]
	];
}

FReply SEOSControlPanel::OnTabClicked(int32 TabIndex)
{
	ActiveTabIndex = TabIndex;
	return FReply::Handled();
}

FSlateColor SEOSControlPanel::GetTabButtonColor(int32 TabIndex) const
{
	if (ActiveTabIndex == TabIndex)
	{
		return FSlateColor(FLinearColor::White);
	}
	return FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f));
}

FSlateFontInfo SEOSControlPanel::GetTabButtonFont(int32 TabIndex) const
{
	if (ActiveTabIndex == TabIndex)
	{
		return FCoreStyle::GetDefaultFontStyle("Bold", 11);
	}
	return FCoreStyle::GetDefaultFontStyle("Regular", 11);
}

#undef LOCTEXT_NAMESPACE
