// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Package Tab (EOS overlay post-package setup)

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SEditableTextBox;
class SVerticalBox;

/**
 * Post-package EOS overlay step: bundles Epic's EOS service installer into the
 * packaged build and generates Play.exe via EOSBootstrapperTool, both taken from
 * the plugin's Dependencies folder.
 */
class SEOSPackageTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEOSPackageTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedPtr<SEditableTextBox> PackageDirBox;
	TSharedPtr<SVerticalBox> StatusBox;

	/** Accumulated output of the last run, shown in the log area. */
	FString LogText;

	void RefreshStatus();
	void AppendLog(const FString& Line);

	FReply OnBrowseClicked();
	FReply OnOpenDependenciesClicked();
	FReply OnRefreshClicked();
	FReply OnSetupClicked();
};
