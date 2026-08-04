// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Setup Tab

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "EOSArtifactsParser.h"

class SEditableTextBox;

class SEOSSetupTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEOSSetupTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// Credential fields
	TSharedPtr<SEditableTextBox> ProductIdBox;
	TSharedPtr<SEditableTextBox> SandboxIdBox;
	TSharedPtr<SEditableTextBox> DeploymentIdBox;
	TSharedPtr<SEditableTextBox> ClientIdBox;
	TSharedPtr<SEditableTextBox> ClientSecretBox;

	// DevAuthTool fields
	TSharedPtr<SEditableTextBox> DevAuthPortBox;
	TSharedPtr<SEditableTextBox> Credential1Box;
	TSharedPtr<SEditableTextBox> Credential2Box;
	TSharedPtr<SEditableTextBox> Credential3Box;
	TSharedPtr<SEditableTextBox> Credential4Box;

	// Last-saved credentials for restart detection
	FEOSArtifactCredentials SavedCredentials;

	// Restart banner
	bool bShowRestartBanner = false;

	// All credential fields render as password dots unless this is on ("Show values"),
	// so the tab is always safe to screenshot or stream.
	bool bRevealCredentials = false;
	bool AreCredentialsMasked() const { return !bRevealCredentials; }

	void LoadCurrentValues();
	FReply OnSaveClicked();
	EVisibility GetRestartBannerVisibility() const;

	TSharedRef<SWidget> MakeFieldRow(const FText& Label, TSharedPtr<SEditableTextBox>& OutTextBox, const FString& InitialValue, bool bIsPassword = false);
};
