// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Editor Module Implementation

#include "EOSMulticlientDevKitEditor.h"
#include "EOSConfigValidator.h"
#include "EOSMulticlientDevKitStyle.h"
#include "SEOSControlPanel.h"
#include "EOSMulticlientDevKitEditorLog.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "Misc/MessageDialog.h"
#include "Logging/MessageLog.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Styling/AppStyle.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"

DEFINE_LOG_CATEGORY(LogEOSMulticlientDevKit);

#define LOCTEXT_NAMESPACE "EOSMulticlientDevKitEditor"

const FName FEOSMulticlientDevKitEditorModule::ControlPanelTabId = FName("EOSMulticlientDevKitControlPanel");

void FEOSMulticlientDevKitEditorModule::StartupModule()
{
	// Icon brushes must exist before anything references them
	FEOSMulticlientDevKitStyle::Initialize();

	// Register nomad tab spawner (available before menus)
	RegisterNomadTab();

	// Register tools menu + toolbar after UToolMenus is ready
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FEOSMulticlientDevKitEditorModule::RegisterToolsMenu));
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FEOSMulticlientDevKitEditorModule::RegisterToolbarButton));

	// Run startup config validation
	RunStartupValidation();
}

void FEOSMulticlientDevKitEditorModule::ShutdownModule()
{
	if (FGlobalTabmanager::Get()->HasTabSpawner(ControlPanelTabId))
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ControlPanelTabId);
	}

	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FEOSMulticlientDevKitStyle::Shutdown();
}

void FEOSMulticlientDevKitEditorModule::RegisterNomadTab()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ControlPanelTabId,
		FOnSpawnTab::CreateRaw(this, &FEOSMulticlientDevKitEditorModule::SpawnControlPanelTab))
		.SetDisplayName(LOCTEXT("ControlPanelTabTitle", "EOS Multiclient Dev Kit"))
		.SetTooltipText(LOCTEXT("ControlPanelTabTooltip", "EOS Multiclient Dev Kit Control Panel"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetIcon(FSlateIcon(FEOSMulticlientDevKitStyle::GetStyleSetName(), "EOSMulticlientDevKit.Icon"));
}

TSharedRef<SDockTab> FEOSMulticlientDevKitEditorModule::SpawnControlPanelTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(NomadTab)
		[
			SNew(SEOSControlPanel)
		];
}

void FEOSMulticlientDevKitEditorModule::OpenControlPanel()
{
	FGlobalTabmanager::Get()->TryInvokeTab(ControlPanelTabId);
}

void FEOSMulticlientDevKitEditorModule::RegisterToolbarButton()
{
	// Owner scope ties the entries to this module so ShutdownModule's UnregisterOwner(this)
	// actually removes them — without it, hot reload leaves stale entries bound to unloaded code.
	FToolMenuOwnerScoped OwnerScoped(this);

	// ExtendMenu never returns null — it creates a placeholder menu if the path isn't
	// registered yet (so a future engine rename means a missing button, not a crash).
	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("EOSMulticlientDevKit");

	Section.AddEntry(FToolMenuEntry::InitToolBarButton(
		"OpenEOSControlPanel",
		FUIAction(FExecuteAction::CreateStatic(&FEOSMulticlientDevKitEditorModule::OpenControlPanel)),
		LOCTEXT("ToolbarButtonLabel", "EOS"),
		LOCTEXT("ToolbarButtonTooltip", "Open EOS Multiclient Dev Kit Control Panel"),
		FSlateIcon(FEOSMulticlientDevKitStyle::GetStyleSetName(), "EOSMulticlientDevKit.Icon.Toolbar")
	));
}

void FEOSMulticlientDevKitEditorModule::RegisterToolsMenu()
{
	// See RegisterToolbarButton for why the owner scope matters.
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = Menu->AddSection("EOSMulticlientDevKit", LOCTEXT("EOSMulticlientDevKitSection", "EOS Multiclient Dev Kit"));

	// Open Control Panel (first entry)
	Section.AddMenuEntry(
		"OpenControlPanel",
		LOCTEXT("OpenControlPanel", "Open Control Panel"),
		LOCTEXT("OpenControlPanelTooltip", "Open the EOS Multiclient Dev Kit Control Panel"),
		FSlateIcon(FEOSMulticlientDevKitStyle::GetStyleSetName(), "EOSMulticlientDevKit.Icon"),
		FUIAction(FExecuteAction::CreateStatic(&FEOSMulticlientDevKitEditorModule::OpenControlPanel))
	);

	Section.AddMenuEntry(
		"ValidateEOSConfig",
		LOCTEXT("ValidateEOSConfig", "Validate EOS Config"),
		LOCTEXT("ValidateEOSConfigTooltip", "Check DefaultEngine.ini for common EOS configuration problems"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FEOSMulticlientDevKitEditorModule::OnValidateConfigClicked))
	);

	Section.AddMenuEntry(
		"OpenDefaultEngineIni",
		LOCTEXT("OpenDefaultEngineIni", "Open DefaultEngine.ini"),
		LOCTEXT("OpenDefaultEngineIniTooltip", "Open DefaultEngine.ini in your default text editor"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FEOSMulticlientDevKitEditorModule::OnOpenDefaultEngineIniClicked))
	);

	Section.AddMenuEntry(
		"OpenEOSPortal",
		LOCTEXT("OpenEOSPortal", "Open EOS Developer Portal"),
		LOCTEXT("OpenEOSPortalTooltip", "Open the EOS Developer Portal in your browser"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FEOSMulticlientDevKitEditorModule::OnOpenEOSPortalClicked))
	);

	Section.AddMenuEntry(
		"OpenDocumentation",
		LOCTEXT("OpenDocumentation", "Open Documentation"),
		LOCTEXT("OpenDocumentationTooltip", "Open EOS Multiclient Dev Kit documentation"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FEOSMulticlientDevKitEditorModule::OnOpenDocumentationClicked))
	);
}

void FEOSMulticlientDevKitEditorModule::RunStartupValidation()
{
	TArray<FEOSConfigDiagnostic> Diagnostics;
	FEOSConfigValidator::Validate(Diagnostics);

	int32 ErrorCount = 0;
	int32 WarningCount = 0;

	for (const FEOSConfigDiagnostic& Diag : Diagnostics)
	{
		if (Diag.Severity == EMessageSeverity::Error)
		{
			ErrorCount++;
		}
		else
		{
			WarningCount++;
		}
	}

	if (ErrorCount > 0 || WarningCount > 0)
	{
		FString ToastMessage = FString::Printf(TEXT("EOS Multiclient Dev Kit: %d error(s), %d warning(s) found in EOS config. Use Tools > EOS Multiclient Dev Kit > Validate EOS Config for details."), ErrorCount, WarningCount);

		FNotificationInfo Info(FText::FromString(ToastMessage));
		Info.ExpireDuration = 8.0f;
		Info.Image = FCoreStyle::Get().GetBrush(ErrorCount > 0 ? TEXT("Icons.ErrorWithColor") : TEXT("Icons.WarningWithColor"));

		FSlateNotificationManager::Get().AddNotification(Info);

		UE_LOG(LogEOSMulticlientDevKit, Warning, TEXT("%s"), *ToastMessage);
	}
}

void FEOSMulticlientDevKitEditorModule::OnValidateConfigClicked()
{
	TArray<FEOSConfigDiagnostic> Diagnostics;
	FEOSConfigValidator::Validate(Diagnostics);

	FMessageLog MessageLog("EOSMulticlientDevKit");
	MessageLog.NewPage(LOCTEXT("ValidationPageTitle", "EOS Config Validation"));

	if (Diagnostics.Num() == 0)
	{
		MessageLog.Info(LOCTEXT("NoIssuesFound", "EOS configuration looks good — no issues found."));
	}
	else
	{
		for (const FEOSConfigDiagnostic& Diag : Diagnostics)
		{
			TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(Diag.Severity);
			Message->AddToken(FTextToken::Create(FText::FromString(Diag.Message)));
			MessageLog.AddMessage(Message);
		}
	}

	MessageLog.Open();
}

void FEOSMulticlientDevKitEditorModule::OnOpenDefaultEngineIniClicked()
{
	FString IniPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir() / TEXT("DefaultEngine.ini"));
	FPlatformProcess::LaunchURL(*IniPath, nullptr, nullptr);
}

void FEOSMulticlientDevKitEditorModule::OnOpenEOSPortalClicked()
{
	FPlatformProcess::LaunchURL(TEXT("https://dev.epicgames.com/portal"), nullptr, nullptr);
}

void FEOSMulticlientDevKitEditorModule::OnOpenDocumentationClicked()
{
	// The plugin's own README, wherever the plugin is installed; the repo page as fallback.
	FString ReadmePath;
	if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("EOSMulticlientDevKit")))
	{
		ReadmePath = FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir() / TEXT("README.md"));
	}

	if (!ReadmePath.IsEmpty() && FPaths::FileExists(ReadmePath))
	{
		FPlatformProcess::LaunchURL(*ReadmePath, nullptr, nullptr);
	}
	else
	{
		FPlatformProcess::LaunchURL(TEXT("https://github.com/official-rimacc/EOSMulticlientDevKit#readme"), nullptr, nullptr);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FEOSMulticlientDevKitEditorModule, EOSMulticlientDevKitEditor)
