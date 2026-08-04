// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Package Tab (EOS overlay post-package setup)

#include "SEOSPackageTab.h"
#include "EOSMulticlientDevKitEditorLog.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/CoreStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "DesktopPlatformModule.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "EOSMulticlientDevKitEditor"

namespace
{
	FString GetDependenciesDir()
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("EOSMulticlientDevKit"));
		return Plugin.IsValid()
			? FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir() / TEXT("Dependencies"))
			: FString();
	}

	FString GetDefaultPackageDir()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("StagedBuilds") / TEXT("Windows"));
	}

	/** Top-level .exe files in the packaged build, minus our own Play.exe output. */
	TArray<FString> FindGameExes(const FString& PackageDir)
	{
		TArray<FString> Files;
		IFileManager::Get().FindFiles(Files, *(PackageDir / TEXT("*.exe")), true, false);
		Files.RemoveAll([](const FString& File) { return File.Equals(TEXT("Play.exe"), ESearchCase::IgnoreCase); });
		return Files;
	}

	TSharedRef<SWidget> MakeStatusRow(bool bOk, const FString& Message)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Top)
			.Padding(0, 2, 8, 0)
			[
				SNew(SImage)
				.Image(FCoreStyle::Get().GetBrush(bOk ? TEXT("Icons.SuccessWithColor") : TEXT("Icons.ErrorWithColor")))
				.DesiredSizeOverride(FVector2D(16, 16))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Top)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Message))
				.ColorAndOpacity(bOk ? FLinearColor::Green : FLinearColor(1.0f, 0.3f, 0.3f))
				.AutoWrapText(true)
			];
	}
}

void SEOSPackageTab::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(16)
		[
			SNew(SVerticalBox)

			// Header
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PackageHeader", "Packaged Build — EOS Overlay"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
			]

			// Description
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 12)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PackageDesc",
					"The EOS overlay loads from a per-machine EOS runtime. Dev machines with the Epic Games Launcher "
					"already have it — players without the launcher do not, so shipped builds need this step.\n"
					"After packaging (Platforms > Windows > Package Project), it bundles the EOS service installer "
					"into the build and generates Play.exe wrapping your game.\n"
					"Ship the entire folder — players launch Play.exe, and the first launch installs the EOS service silently."))
				.AutoWrapText(true)
				.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
			]

			// Status checklist + refresh / open-folder buttons
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PackageStatusHeader", "Status"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 8, 0)
				[
					SNew(SButton)
					.OnClicked(this, &SEOSPackageTab::OnOpenDependenciesClicked)
					.ContentPadding(FMargin(12, 4))
					[
						SNew(STextBlock).Text(LOCTEXT("OpenDeps", "Open Dependencies Folder"))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.OnClicked(this, &SEOSPackageTab::OnRefreshClicked)
					.ContentPadding(FMargin(12, 4))
					[
						SNew(STextBlock).Text(LOCTEXT("RefreshStatus", "Refresh"))
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 12)
			[
				SAssignNew(StatusBox, SVerticalBox)
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12) [ SNew(SSeparator) ]

			// Package dir row
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 12, 0)
				[
					SNew(SBox)
					.WidthOverride(140)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("PackageDirLabel", "Packaged build folder"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SAssignNew(PackageDirBox, SEditableTextBox)
					.Text(FText::FromString(GetDefaultPackageDir()))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(8, 0, 0, 0)
				[
					SNew(SButton)
					.OnClicked(this, &SEOSPackageTab::OnBrowseClicked)
					.ContentPadding(FMargin(12, 4))
					[
						SNew(STextBlock).Text(LOCTEXT("Browse", "Browse..."))
					]
				]
			]

			// The one button
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8, 0, 12)
			[
				SNew(SButton)
				.OnClicked(this, &SEOSPackageTab::OnSetupClicked)
				.ContentPadding(FMargin(32, 10))
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SetupOverlayButton", "Set Up Overlay For Packaged Build"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				]
			]

			// Output log
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return FText::FromString(LogText); })
				.Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
				.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
				.AutoWrapText(true)
			]
		]
	];

	RefreshStatus();
}

void SEOSPackageTab::RefreshStatus()
{
	StatusBox->ClearChildren();

	const FString Deps = GetDependenciesDir();
	const FString PackageDir = PackageDirBox.IsValid() && !PackageDirBox->GetText().IsEmpty()
		? PackageDirBox->GetText().ToString()
		: GetDefaultPackageDir();

	struct FToolCheck
	{
		const TCHAR* File;
		bool bPresent;
	};
	const FToolCheck Tools[] =
	{
		{ TEXT("EOSBootstrapperTool.exe"), IFileManager::Get().FileExists(*(Deps / TEXT("EOSBootstrapperTool.exe"))) },
		{ TEXT("EOSBootstrapper.exe"), IFileManager::Get().FileExists(*(Deps / TEXT("EOSBootstrapper.exe"))) },
		{ TEXT("EpicOnlineServicesInstaller.exe"), IFileManager::Get().FileExists(*(Deps / TEXT("EpicOnlineServicesInstaller.exe"))) },
	};

	for (const FToolCheck& Tool : Tools)
	{
		const FString Message = Tool.bPresent
			? FString::Printf(TEXT("%s found in Dependencies."), Tool.File)
			: FString::Printf(TEXT("%s missing — drop it into the Dependencies folder (from the EOS SDK's SDK/Tools). See Dependencies/README.md."), Tool.File);
		StatusBox->AddSlot().AutoHeight().Padding(0, 2) [ MakeStatusRow(Tool.bPresent, Message) ];
	}

	// Staged build presence + game exe detection
	if (!IFileManager::Get().DirectoryExists(*PackageDir))
	{
		StatusBox->AddSlot().AutoHeight().Padding(0, 2)
			[ MakeStatusRow(false, FString::Printf(TEXT("No packaged build at %s — package the project first (Platforms > Windows > Package Project)."), *PackageDir)) ];
	}
	else
	{
		const TArray<FString> Exes = FindGameExes(PackageDir);
		if (Exes.Num() == 1)
		{
			StatusBox->AddSlot().AutoHeight().Padding(0, 2)
				[ MakeStatusRow(true, FString::Printf(TEXT("Packaged build found — game exe: %s"), *Exes[0])) ];
		}
		else if (Exes.Num() == 0)
		{
			StatusBox->AddSlot().AutoHeight().Padding(0, 2)
				[ MakeStatusRow(false, FString::Printf(TEXT("Folder exists but no game .exe found at its top level: %s"), *PackageDir)) ];
		}
		else
		{
			StatusBox->AddSlot().AutoHeight().Padding(0, 2)
				[ MakeStatusRow(false, FString::Printf(TEXT("Several .exe files found (%s) — remove the extras so the game exe is unambiguous."), *FString::Join(Exes, TEXT(", ")))) ];
		}
	}
}

void SEOSPackageTab::AppendLog(const FString& Line)
{
	LogText += Line + TEXT("\n");
	UE_LOG(LogEOSMulticlientDevKit, Log, TEXT("Package: %s"), *Line);
}

FReply SEOSPackageTab::OnBrowseClicked()
{
	if (IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get())
	{
		const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(AsShared());
		FString Folder;
		if (DesktopPlatform->OpenDirectoryDialog(
				ParentWindowHandle,
				LOCTEXT("BrowsePackageDir", "Select the packaged build folder").ToString(),
				PackageDirBox->GetText().ToString(),
				Folder))
		{
			PackageDirBox->SetText(FText::FromString(FPaths::ConvertRelativePathToFull(Folder)));
			RefreshStatus();
		}
	}
	return FReply::Handled();
}

FReply SEOSPackageTab::OnOpenDependenciesClicked()
{
	const FString Deps = GetDependenciesDir();
	if (!Deps.IsEmpty())
	{
		IFileManager::Get().MakeDirectory(*Deps, true);
		FPlatformProcess::ExploreFolder(*Deps);
	}
	return FReply::Handled();
}

FReply SEOSPackageTab::OnRefreshClicked()
{
	RefreshStatus();
	return FReply::Handled();
}

FReply SEOSPackageTab::OnSetupClicked()
{
	LogText.Reset();
	RefreshStatus();

	const FString Deps = GetDependenciesDir();
	const FString PackageDir = PackageDirBox->GetText().ToString();
	const FString BootstrapperTool = Deps / TEXT("EOSBootstrapperTool.exe");
	const FString Bootstrapper = Deps / TEXT("EOSBootstrapper.exe");
	const FString Installer = Deps / TEXT("EpicOnlineServicesInstaller.exe");

	auto Fail = [this](const FString& Message) -> FReply
	{
		AppendLog(FString::Printf(TEXT("[ERROR] %s"), *Message));
		FNotificationInfo Info(FText::Format(LOCTEXT("OverlaySetupFailed", "EOS Multiclient Dev Kit: Overlay setup failed — {0}"), FText::FromString(Message)));
		Info.ExpireDuration = 6.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
		FSlateNotificationManager::Get().AddNotification(Info);
		return FReply::Handled();
	};

	// Validate inputs
	if (!IFileManager::Get().FileExists(*BootstrapperTool) ||
		!IFileManager::Get().FileExists(*Bootstrapper) ||
		!IFileManager::Get().FileExists(*Installer))
	{
		return Fail(TEXT("Epic's tools are missing from the Dependencies folder. See the status list above."));
	}
	if (!IFileManager::Get().DirectoryExists(*PackageDir))
	{
		return Fail(FString::Printf(TEXT("Packaged build folder not found: %s"), *PackageDir));
	}

	const TArray<FString> Exes = FindGameExes(PackageDir);
	if (Exes.Num() == 0)
	{
		return Fail(TEXT("No game .exe found at the top level of the packaged build folder."));
	}
	if (Exes.Num() > 1)
	{
		return Fail(FString::Printf(TEXT("Several .exe files found (%s) — remove the extras so the game exe is unambiguous."), *FString::Join(Exes, TEXT(", "))));
	}
	const FString GameExe = Exes[0];

	AppendLog(FString::Printf(TEXT("Package dir: %s"), *PackageDir));
	AppendLog(FString::Printf(TEXT("Game exe:    %s"), *GameExe));
	AppendLog(TEXT(""));

	FScopedSlowTask SlowTask(3.0f, LOCTEXT("OverlaySetupProgress", "Setting up EOS overlay for the packaged build..."));
	SlowTask.MakeDialog();

	// [1/3] Bundle the EOS service installer
	SlowTask.EnterProgressFrame(1.0f, LOCTEXT("OverlayStep1", "Copying EOS service installer..."));
	const FString InstallerDestDir = PackageDir / TEXT("EpicOnlineServices");
	const FString InstallerDest = InstallerDestDir / TEXT("EpicOnlineServicesInstaller.exe");
	IFileManager::Get().MakeDirectory(*InstallerDestDir, true);
	if (IFileManager::Get().Copy(*InstallerDest, *Installer) != COPY_OK)
	{
		return Fail(TEXT("Failed to copy EpicOnlineServicesInstaller.exe into the build."));
	}
	AppendLog(TEXT("[1/3] OK — EpicOnlineServices/EpicOnlineServicesInstaller.exe"));

	// [2/3] Generate Play.exe via Epic's bootstrapper tool
	SlowTask.EnterProgressFrame(1.0f, LOCTEXT("OverlayStep2", "Generating EOS bootstrapper (Play.exe)..."));
	const FString PlayExe = PackageDir / TEXT("Play.exe");
	const FString Params = FString::Printf(TEXT("--output-path \"%s\" --app-path \"%s\""), *PlayExe, *GameExe);
	AppendLog(FString::Printf(TEXT("[2/3] EOSBootstrapperTool %s"), *Params));

	int32 ReturnCode = -1;
	FString StdOut, StdErr;
	const bool bLaunched = FPlatformProcess::ExecProcess(*BootstrapperTool, *Params, &ReturnCode, &StdOut, &StdErr, *PackageDir);
	if (!StdOut.TrimStartAndEnd().IsEmpty()) { AppendLog(StdOut.TrimStartAndEnd()); }
	if (!StdErr.TrimStartAndEnd().IsEmpty()) { AppendLog(StdErr.TrimStartAndEnd()); }
	if (!bLaunched || ReturnCode != 0)
	{
		return Fail(FString::Printf(TEXT("EOSBootstrapperTool failed (exit code %d). See the log below."), ReturnCode));
	}
	AppendLog(TEXT("[2/3] OK"));

	// [3/3] Verify
	SlowTask.EnterProgressFrame(1.0f, LOCTEXT("OverlayStep3", "Verifying..."));
	if (!IFileManager::Get().FileExists(*PlayExe) || !IFileManager::Get().FileExists(*InstallerDest))
	{
		return Fail(TEXT("Verification failed — Play.exe or the bundled installer is missing."));
	}
	AppendLog(TEXT("[3/3] OK — all files in place"));
	AppendLog(TEXT(""));
	AppendLog(TEXT("Done. Ship the ENTIRE folder. Players launch Play.exe — NOT ") + GameExe + TEXT("."));
	AppendLog(TEXT("First launch on a machine installs the EOS service silently; every launch after is instant."));

	RefreshStatus();

	FNotificationInfo Info(LOCTEXT("OverlaySetupDone", "EOS Multiclient Dev Kit: Overlay set up — ship the whole folder; players launch Play.exe."));
	Info.ExpireDuration = 6.0f;
	Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.SuccessWithColor"));
	FSlateNotificationManager::Get().AddNotification(Info);

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
