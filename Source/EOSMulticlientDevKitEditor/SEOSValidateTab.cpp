// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Validate Tab

#include "SEOSValidateTab.h"
#include "EOSConfigValidator.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "EOSMulticlientDevKitEditor"

void SEOSValidateTab::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)

		// Header + Re-validate button
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 12)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ValidateHeader", "EOS Configuration Checks"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked(this, &SEOSValidateTab::OnRevalidateClicked)
				.ContentPadding(FMargin(16, 6))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Revalidate", "Re-validate"))
				]
			]
		]

		// Results list (scrollable)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(ResultsBox, SVerticalBox)
			]
		]
	];

	// Run validation on construct
	RunValidation();
}

FReply SEOSValidateTab::OnRevalidateClicked()
{
	RunValidation();
	return FReply::Handled();
}

void SEOSValidateTab::RunValidation()
{
	ResultsBox->ClearChildren();

	TArray<FEOSConfigDiagnostic> Diagnostics;
	const FEOSValidationSummary Summary = FEOSConfigValidator::Validate(Diagnostics);

	if (Diagnostics.Num() == 0)
	{
		ResultsBox->AddSlot()
		.AutoHeight()
		.Padding(16, 8)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 8, 0)
			[
				SNew(SImage)
				.Image(FCoreStyle::Get().GetBrush(TEXT("Icons.SuccessWithColor")))
				.DesiredSizeOverride(FVector2D(16, 16))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AllChecksPassed", "All checks passed \u2014 no issues found."))
				.ColorAndOpacity(FLinearColor::Green)
			]
		];
	}
	else
	{
		for (const FEOSConfigDiagnostic& Diag : Diagnostics)
		{
			const FName IconName = (Diag.Severity == EMessageSeverity::Error)
				? TEXT("Icons.ErrorWithColor")
				: TEXT("Icons.WarningWithColor");

			const FLinearColor TextColor = (Diag.Severity == EMessageSeverity::Error)
				? FLinearColor(1.0f, 0.3f, 0.3f)
				: FLinearColor(1.0f, 0.8f, 0.2f);

			ResultsBox->AddSlot()
			.AutoHeight()
			.Padding(16, 4)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Top)
				.Padding(0, 2, 8, 0)
				[
					SNew(SImage)
					.Image(FCoreStyle::Get().GetBrush(IconName))
					.DesiredSizeOverride(FVector2D(16, 16))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Diag.Message))
					.ColorAndOpacity(TextColor)
					.AutoWrapText(true)
				]
			];
		}
	}

	// Summary line
	const FLinearColor SummaryColor = (Diagnostics.Num() == 0)
		? FLinearColor::Green
		: FLinearColor(0.6f, 0.6f, 0.6f);

	ResultsBox->AddSlot()
	.AutoHeight()
	.Padding(16, 12, 16, 8)
	[
		SNew(STextBlock)
		.Text(FText::FromString(FString::Printf(TEXT("%d of %d checks passed."), Summary.PassedChecks, Summary.TotalChecks)))
		.ColorAndOpacity(SummaryColor)
	];
}

#undef LOCTEXT_NAMESPACE
