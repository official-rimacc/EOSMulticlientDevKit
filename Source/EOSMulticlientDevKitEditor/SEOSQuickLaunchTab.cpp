// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Quick Launch Tab

#include "SEOSQuickLaunchTab.h"
#include "EOSMulticlientDevKitEditorLog.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Styling/CoreStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"

#define LOCTEXT_NAMESPACE "EOSMulticlientDevKitEditor"

static const TCHAR* DevAuthSection = TEXT("EOSMulticlientDevKit.DevAuthTool");
static const TCHAR* DefaultDevAuthPort = TEXT("8081");

/** Read credential names from [EOSMulticlientDevKit.DevAuthTool]. */
static TArray<FString> GetConfiguredCredentials()
{
	TArray<FString> Credentials;
	const TArray<FString> DefaultNames = { TEXT("Player1"), TEXT("Player2"), TEXT("Player3"), TEXT("Player4") };

	for (int32 i = 0; i < 4; ++i)
	{
		FString Key = FString::Printf(TEXT("Player%dCredential"), i + 1);
		FString Value;
		if (!GConfig->GetString(DevAuthSection, *Key, Value, GEngineIni) || Value.IsEmpty())
		{
			Value = DefaultNames[i];
		}
		Credentials.Add(Value);
	}

	return Credentials;
}

/** DevAuthTool port, as configured on the Setup tab. */
static FString GetDevAuthPort()
{
	FString Port;
	if (!GConfig->GetString(DevAuthSection, TEXT("Port"), Port, GEngineIni) || Port.IsEmpty())
	{
		Port = DefaultDevAuthPort;
	}
	return Port;
}

/** Build the credential mapping text dynamically from ini values. */
static FText BuildCredentialInfoText()
{
	TArray<FString> Creds = GetConfiguredCredentials();
	FString Info = FString::Printf(
		TEXT("In the DevAuthTool (localhost:%s), sign in one account per slot and name each login EXACTLY:"),
		*GetDevAuthPort());
	for (int32 i = 0; i < Creds.Num(); ++i)
	{
		Info += FString::Printf(TEXT("\n    Player %d  ->  \"%s\""), i + 1, *Creds[i]);
	}
	Info += TEXT("\n\nNames are set on the Setup tab. A client whose name has no matching");
	Info += TEXT("\nDevAuthTool login will start, but stay signed out.");
	return FText::FromString(Info);
}

void SEOSQuickLaunchTab::Construct(const FArguments& InArgs)
{
	auto MakeLaunchButton = [this](int32 NumPlayers, const FText& Label) -> TSharedRef<SWidget>
	{
		return SNew(SButton)
			.OnClicked(this, &SEOSQuickLaunchTab::OnLaunchStandalone, NumPlayers)
			.ContentPadding(FMargin(24, 12))
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			];
	};

	ChildSlot
	[
		SNew(SVerticalBox)

		// Header
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 16, 16, 4)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("QuickLaunchHeader", "Quick Launch"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
		]

		// Description
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 0, 16, 8)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("QuickLaunchDesc",
				"Launch standalone game instances with separate EOS credentials.\n"
				"Each client gets its own process, its own EOS overlay and its own DevAuthTool login.\n"
				"Your game must call IOnlineIdentity::AutoLogin() — see the plugin README."))
			.AutoWrapText(true)
			.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
		]

		// The requirement people miss: DevAuthTool is a local login server that must be
		// running and signed in for EVERY test session, not a one-time setup step.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 0, 16, 16)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("QuickLaunchDevAuthWarning",
				"Start the DevAuthTool and sign in BEFORE launching — every session. It is a local "
				"login server these clients authenticate against; if it isn't running, clients start "
				"but can't sign in."))
			.AutoWrapText(true)
			.ColorAndOpacity(FLinearColor(1.0f, 0.8f, 0.2f))
		]

		// Launch buttons
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 8)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
			[
				MakeLaunchButton(2, LOCTEXT("Launch2", "Test With 2 Players"))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
			[
				MakeLaunchButton(3, LOCTEXT("Launch3", "Test With 3 Players"))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeLaunchButton(4, LOCTEXT("Launch4", "Test With 4 Players"))
			]
		]

		// Guest option — last launched client uses anonymous Device ID login instead of DevAuthTool.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 12, 16, 0)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return bLastClientIsGuest ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bLastClientIsGuest = (NewState == ECheckBoxState::Checked); })
			.ToolTipText(LOCTEXT("GuestClientTooltip",
				"ADVANCED — requires game-side support. The last launched client is passed -EosGuestLogin instead of "
				"DevAuthTool credentials, for projects that implement an anonymous EOS Device ID login. Stock UE 5.7 has "
				"no Device ID path, so this does nothing unless your project provides one.\nOnly one guest per machine: "
				"the device id is bound to your Windows user, so a second guest would get the SAME ProductUserId and "
				"collide.\nGuests have no Epic overlay/invites."))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("GuestClientLabel", "Last client = Guest (Device ID — requires game-side support)"))
			]
		]

		// Separator
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 12)
		[
			SNew(SSeparator)
		]

		// Credential mapping info (read from ini)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 0)
		[
			SNew(STextBlock)
			// Bound, not snapshotted: the Setup tab can change these while this tab exists.
			.Text_Lambda([]() { return BuildCredentialInfoText(); })
			.Font(FCoreStyle::GetDefaultFontStyle("Mono", 10))
			.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
		]
	];
}

FReply SEOSQuickLaunchTab::OnLaunchStandalone(int32 NumPlayers)
{
	TArray<FString> Credentials = GetConfiguredCredentials();

	// Tile windows side-by-side (2 columns)
	const int32 WinW = 1280;
	const int32 WinH = 720;
	const int32 StartX = 50;
	const int32 StartY = 50;

	for (int32 i = 0; i < NumPlayers && i < Credentials.Num(); ++i)
	{
		int32 Col = i % 2;
		int32 Row = i / 2;
		int32 WinX = StartX + Col * (WinW + 20);
		int32 WinY = StartY + Row * (WinH + 50);

		// Optionally make the LAST instance an anonymous Device ID guest (max one per machine —
		// the device id is per-Windows-user, so a second guest would share the same PUID).
		const bool bAsGuest = bLastClientIsGuest && (i == NumPlayers - 1);

		LaunchStandaloneProcess(Credentials[i], bAsGuest, WinX, WinY, WinW, WinH);
	}

	UE_LOG(LogEOSMulticlientDevKit, Log, TEXT("Quick Launch: Launched %d standalone client(s)"), NumPlayers);

	FNotificationInfo Info(FText::Format(
		LOCTEXT("LaunchedNotification", "EOS Multiclient Dev Kit: Launched {0} standalone client(s)."),
		FText::AsNumber(NumPlayers)));
	Info.ExpireDuration = 4.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	return FReply::Handled();
}

void SEOSQuickLaunchTab::LaunchStandaloneProcess(const FString& Credential, bool bAsGuest, int32 WinX, int32 WinY, int32 ResX, int32 ResY)
{
	FString EditorBinary = FPlatformProcess::ExecutablePath();
	FString ProjectPath = FPaths::GetProjectFilePath();

	// Normal client: the stock AUTH_* switches. FUserManagerEOS::AutoLogin parses these straight off the
	// command line, so any project that calls IOnlineIdentity::AutoLogin() logs in with no extra glue.
	// -EosCredential is also passed for projects that drive the login themselves and want the raw name.
	//
	// Guest: -EosGuestLogin only — there is no stock Device ID path to hand credentials to, so this is
	// inert unless the project implements it (see the checkbox tooltip and the README).
	FString AuthArgs;
	if (bAsGuest)
	{
		AuthArgs = TEXT("-EosGuestLogin");
	}
	else
	{
		// Credential names are quoted so a name with a space survives argument parsing
		// (FParse::Value on the receiving end unquotes them).
		AuthArgs = FString::Printf(
			TEXT("-AUTH_TYPE=developer -AUTH_LOGIN=localhost:%s -AUTH_PASSWORD=\"%s\" -EosCredential=\"%s\""),
			*GetDevAuthPort(), *Credential, *Credential);
	}

	// Per-instance log file (Saved/Logs/DevKit_<who>.log) so multi-client runs don't collide into
	// anonymous <Project>_2.log files; -LOGTIMES adds wall-clock timestamps to the console too.
	FString LogName = bAsGuest ? TEXT("Guest") : Credential;
	LogName = LogName.Replace(TEXT(" "), TEXT("_"));

	FString Args = FString::Printf(
		TEXT("\"%s\" -game %s -windowed -WinX=%d -WinY=%d -ResX=%d -ResY=%d -log LOG=DevKit_%s.log -LOGTIMES"),
		*ProjectPath,
		*AuthArgs,
		WinX, WinY,
		ResX, ResY,
		*LogName
	);

	UE_LOG(LogEOSMulticlientDevKit, Log, TEXT("Quick Launch: %s %s"), *EditorBinary, *Args);

	FProcHandle Handle = FPlatformProcess::CreateProc(
		*EditorBinary, *Args,
		true,   // bLaunchDetached
		false,  // bLaunchHidden
		false,  // bLaunchReallyHidden
		nullptr, 0, nullptr, nullptr
	);

	if (!Handle.IsValid())
	{
		const FString WhoFailed = bAsGuest ? TEXT("Guest (Device ID)") : Credential;
		UE_LOG(LogEOSMulticlientDevKit, Error, TEXT("Quick Launch: Failed to launch standalone process for '%s'"), *WhoFailed);

		FNotificationInfo Info(FText::Format(
			LOCTEXT("LaunchFailed", "EOS Multiclient Dev Kit: Failed to launch client for \"{0}\"."),
			FText::FromString(WhoFailed)));
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	// The client is detached and never tracked — release our copy of the OS handle.
	FPlatformProcess::CloseProc(Handle);
}

#undef LOCTEXT_NAMESPACE
