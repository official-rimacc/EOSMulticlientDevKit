// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Setup Tab

#include "SEOSSetupTab.h"
#include "EOSConfigWriter.h"
#include "EOSMulticlientDevKitEditorLog.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Styling/CoreStyle.h"
#include "Styling/AppStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Misc/ConfigCacheIni.h"

#define LOCTEXT_NAMESPACE "EOSMulticlientDevKitEditor"

static const TCHAR* DevAuthSection = TEXT("EOSMulticlientDevKit.DevAuthTool");

void SEOSSetupTab::Construct(const FArguments& InArgs)
{
	LoadCurrentValues();

	// Read DevAuthTool settings from ini
	FString DevAuthPort = TEXT("8081");
	FString Cred1 = TEXT("Player1"), Cred2 = TEXT("Player2"), Cred3 = TEXT("Player3"), Cred4 = TEXT("Player4");
	GConfig->GetString(DevAuthSection, TEXT("Port"), DevAuthPort, GEngineIni);
	GConfig->GetString(DevAuthSection, TEXT("Player1Credential"), Cred1, GEngineIni);
	GConfig->GetString(DevAuthSection, TEXT("Player2Credential"), Cred2, GEngineIni);
	GConfig->GetString(DevAuthSection, TEXT("Player3Credential"), Cred3, GEngineIni);
	GConfig->GetString(DevAuthSection, TEXT("Player4Credential"), Cred4, GEngineIni);

	ChildSlot
	[
		SNew(SVerticalBox)

		// Restart Required banner
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.9f, 0.5f, 0.0f))
			.Padding(FMargin(16, 8))
			.Visibility(this, &SEOSSetupTab::GetRestartBannerVisibility)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RestartRequired", "Credential changes may require an editor restart — Quick Launch clients always read the latest saved values, but in-editor EOS features (e.g. PIE) only pick them up after a restart."))
				.ColorAndOpacity(FLinearColor::White)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			]
		]

		// Scrollable content
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			.Padding(16)
			[
				SNew(SVerticalBox)

				// === EOS Credentials Section ===
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 8)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CredentialsHeader", "EOS Credentials"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 12)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CredentialsDesc", "Enter your credentials from the EOS Developer Portal. Saving writes the Artifacts= line and adds any missing recommended EOS config to DefaultEngine.ini — existing lines are never overwritten."))
					.AutoWrapText(true)
					.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ MakeFieldRow(LOCTEXT("ProductId", "Product ID"), ProductIdBox, SavedCredentials.ProductId, true) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ MakeFieldRow(LOCTEXT("SandboxId", "Sandbox ID"), SandboxIdBox, SavedCredentials.SandboxId, true) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ MakeFieldRow(LOCTEXT("DeploymentId", "Deployment ID"), DeploymentIdBox, SavedCredentials.DeploymentId, true) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ MakeFieldRow(LOCTEXT("ClientId", "Client ID"), ClientIdBox, SavedCredentials.ClientId, true) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ MakeFieldRow(LOCTEXT("ClientSecret", "Client Secret"), ClientSecretBox, SavedCredentials.ClientSecret, true) ]

				// Reveal toggle — fields are dots by default so the tab is screenshot/stream safe.
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 8, 0, 0)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return bRevealCredentials ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bRevealCredentials = (NewState == ECheckBoxState::Checked); })
					.ToolTipText(LOCTEXT("RevealCredentialsTooltip",
						"Temporarily show the credential values, e.g. to check what you pasted. "
						"They render as dots by default so screenshots and streams never leak them."))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("RevealCredentials", "Show values"))
					]
				]

				// === DevAuthTool Section ===
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 8) [ SNew(SSeparator) ]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 8, 0, 8)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DevAuthHeader", "DevAuthTool"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 12)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DevAuthDesc", "Configure the DevAuthTool port and credential names for each player slot.\nThese are used by Quick Launch and the editor login flow."))
					.AutoWrapText(true)
					.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ MakeFieldRow(LOCTEXT("DevAuthPort", "Port"), DevAuthPortBox, DevAuthPort) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ MakeFieldRow(LOCTEXT("Cred1", "Player 1 Credential"), Credential1Box, Cred1) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ MakeFieldRow(LOCTEXT("Cred2", "Player 2 Credential"), Credential2Box, Cred2) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ MakeFieldRow(LOCTEXT("Cred3", "Player 3 Credential"), Credential3Box, Cred3) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ MakeFieldRow(LOCTEXT("Cred4", "Player 4 Credential"), Credential4Box, Cred4) ]

				// === Save Button ===
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 16, 0, 0)
				[
					SNew(SButton)
					.OnClicked(this, &SEOSSetupTab::OnSaveClicked)
					.ContentPadding(FMargin(32, 10))
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SaveButton", "Save"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
					]
				]
			]
		]
	];
}

void SEOSSetupTab::LoadCurrentValues()
{
	FEOSArtifactsParser::Load(SavedCredentials);
}

FReply SEOSSetupTab::OnSaveClicked()
{
	// === Save EOS Credentials (Artifacts line via raw file I/O) ===
	FEOSArtifactCredentials NewCredentials;
	NewCredentials.ArtifactName = SavedCredentials.ArtifactName.IsEmpty() ? TEXT("Dev") : SavedCredentials.ArtifactName;
	NewCredentials.ProductId = ProductIdBox->GetText().ToString();
	NewCredentials.SandboxId = SandboxIdBox->GetText().ToString();
	NewCredentials.DeploymentId = DeploymentIdBox->GetText().ToString();
	NewCredentials.ClientId = ClientIdBox->GetText().ToString();
	NewCredentials.ClientSecret = ClientSecretBox->GetText().ToString();
	NewCredentials.ClientEncryptionKey = SavedCredentials.ClientEncryptionKey;

	bool bCredentialsChanged =
		NewCredentials.ProductId != SavedCredentials.ProductId ||
		NewCredentials.SandboxId != SavedCredentials.SandboxId ||
		NewCredentials.DeploymentId != SavedCredentials.DeploymentId ||
		NewCredentials.ClientId != SavedCredentials.ClientId ||
		NewCredentials.ClientSecret != SavedCredentials.ClientSecret;

	bool bSaveOk = FEOSArtifactsParser::Save(NewCredentials);

	// === Fill in the recommended EOS config block (add-only; never rewrites existing lines) ===
	bool bConfigOk = true;
	TArray<FString> AppliedConfigLines;
	if (bSaveOk)
	{
		SavedCredentials = NewCredentials;
		bConfigOk = FEOSConfigWriter::ApplyRecommendedConfig(NewCredentials.ArtifactName, AppliedConfigLines);

		// EOS reads all of this at startup — credential edits and newly added config
		// lines both need a restart to take effect.
		if (bCredentialsChanged || AppliedConfigLines.Num() > 0)
		{
			bShowRestartBanner = true;
		}
	}

	// === Save DevAuthTool settings (GConfig — standard key=value) ===
	GConfig->SetString(DevAuthSection, TEXT("Port"), *DevAuthPortBox->GetText().ToString(), GEngineIni);
	GConfig->SetString(DevAuthSection, TEXT("Player1Credential"), *Credential1Box->GetText().ToString(), GEngineIni);
	GConfig->SetString(DevAuthSection, TEXT("Player2Credential"), *Credential2Box->GetText().ToString(), GEngineIni);
	GConfig->SetString(DevAuthSection, TEXT("Player3Credential"), *Credential3Box->GetText().ToString(), GEngineIni);
	GConfig->SetString(DevAuthSection, TEXT("Player4Credential"), *Credential4Box->GetText().ToString(), GEngineIni);
	GConfig->Flush(false, GEngineIni);

	// === Notification ===
	if (bSaveOk && bConfigOk)
	{
		FNotificationInfo Info(AppliedConfigLines.Num() > 0
			? FText::Format(LOCTEXT("SaveSuccessWithConfig", "EOS Multiclient Dev Kit: Settings saved — added {0} missing EOS config line(s) to DefaultEngine.ini."), FText::AsNumber(AppliedConfigLines.Num()))
			: LOCTEXT("SaveSuccess", "EOS Multiclient Dev Kit: Settings saved."));
		Info.ExpireDuration = 4.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	else
	{
		FNotificationInfo Info(bSaveOk
			? LOCTEXT("ConfigWriteFailed", "EOS Multiclient Dev Kit: Credentials saved, but updating the recommended config in DefaultEngine.ini failed.")
			: LOCTEXT("SaveFailed", "EOS Multiclient Dev Kit: Failed to save credentials."));
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
		FSlateNotificationManager::Get().AddNotification(Info);
	}

	return FReply::Handled();
}

EVisibility SEOSSetupTab::GetRestartBannerVisibility() const
{
	return bShowRestartBanner ? EVisibility::Visible : EVisibility::Collapsed;
}

TSharedRef<SWidget> SEOSSetupTab::MakeFieldRow(const FText& Label, TSharedPtr<SEditableTextBox>& OutTextBox, const FString& InitialValue, bool bIsPassword)
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 12, 0)
		[
			SNew(SBox)
			.WidthOverride(140)
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
			]
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SAssignNew(OutTextBox, SEditableTextBox)
			.Text(FText::FromString(InitialValue))
			// Secret fields follow the "Show values" toggle; others are always plain.
			.IsPassword(bIsPassword
				? TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SEOSSetupTab::AreCredentialsMasked))
				: TAttribute<bool>(false))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
		];
}

#undef LOCTEXT_NAMESPACE
