// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Editor Module Header

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class SDockTab;
class FSpawnTabArgs;

class FEOSMulticlientDevKitEditorModule : public IModuleInterface
{
public:
	static const FName ControlPanelTabId;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static void OpenControlPanel();

private:
	void RegisterToolsMenu();
	void RegisterNomadTab();
	void RegisterToolbarButton();
	void RunStartupValidation();

	TSharedRef<SDockTab> SpawnControlPanelTab(const FSpawnTabArgs& Args);

	static void OnValidateConfigClicked();
	static void OnOpenDefaultEngineIniClicked();
	static void OnOpenEOSPortalClicked();
	static void OnOpenDocumentationClicked();
};
