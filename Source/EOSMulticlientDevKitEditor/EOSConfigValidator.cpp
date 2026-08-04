// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — EOS Configuration Validator

#include "EOSConfigValidator.h"
#include "EOSArtifactsParser.h"
#include "EOSMulticlientDevKitEditorLog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"

namespace
{
	/**
	 * Strip whole comment lines (';' / '#') so substring checks can neither fire on
	 * commented-out config nor let it satisfy a required-setting check.
	 *
	 * Trailing-';' comments are deliberately left alone: UE ini has no trailing-comment
	 * syntax, so a ';' inside a value (a client secret, say) is data, not a comment.
	 */
	FString StripCommentLines(const FString& IniText)
	{
		TArray<FString> Lines;
		IniText.ParseIntoArrayLines(Lines, false);

		FString Result;
		Result.Reserve(IniText.Len());
		for (const FString& Line : Lines)
		{
			const FString Trimmed = Line.TrimStart();
			if (!Trimmed.StartsWith(TEXT(";")) && !Trimmed.StartsWith(TEXT("#")))
			{
				Result += Line;
				Result += TEXT("\n");
			}
		}
		return Result;
	}
}

FEOSValidationSummary FEOSConfigValidator::Validate(TArray<FEOSConfigDiagnostic>& OutDiagnostics)
{
	FEOSValidationSummary Summary;

	// Read raw DefaultEngine.ini text (not GConfig — GConfig silently merges bad section names,
	// which would hide exactly the mistake CheckSectionName is looking for).
	FString IniPath = FPaths::ProjectConfigDir() / TEXT("DefaultEngine.ini");
	FString IniText;

	if (!FFileHelper::LoadFileToString(IniText, *IniPath))
	{
		OutDiagnostics.Emplace(EMessageSeverity::Error,
			TEXT("Could not read DefaultEngine.ini. Expected at: ") + IniPath);
		return Summary;
	}

	// Substring checks run against comment-stripped text. CheckCredentials instead shares
	// FEOSArtifactsParser's live-line lookup, which does its own comment handling.
	const FString CleanIni = StripCommentLines(IniText);

	// Run all checks in order of severity. Each returns true when it found no problem.
	const bool bResults[] =
	{
		CheckCredentials(IniText, OutDiagnostics),                  // #1  - Error
		CheckArtifactNameMatch(IniText, CleanIni, OutDiagnostics),  // #2  - Error
		CheckSectionName(CleanIni, OutDiagnostics),                 // #3  - Error
		CheckAuthScopeFlags(CleanIni, OutDiagnostics),              // #4  - Error
		CheckDefaultPlatformService(CleanIni, OutDiagnostics),      // #5  - Error
		CheckPluginEnabled(TEXT("OnlineSubsystemEOS"), OutDiagnostics),  // #6 - Error
		CheckPluginEnabled(TEXT("SocketSubsystemEOS"), OutDiagnostics),  // #7 - Error
		CheckNetDriverConfig(CleanIni, OutDiagnostics),             // #8  - Warning
		CheckAuthFlags(CleanIni, OutDiagnostics),                   // #9  - Warning
		CheckOverlayFlags(CleanIni, OutDiagnostics),                // #10 - Warning
		CheckEditorOverlay(CleanIni, OutDiagnostics),               // #11 - Warning
		CheckEncryptionKey(CleanIni, OutDiagnostics),               // #12 - Warning
	};

	Summary.TotalChecks = UE_ARRAY_COUNT(bResults);
	for (bool bPassed : bResults)
	{
		Summary.PassedChecks += bPassed ? 1 : 0;
	}

	return Summary;
}

// #1: Missing or placeholder EOS credentials
bool FEOSConfigValidator::CheckCredentials(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics)
{
	// Shares the parser's live-line lookup (comment-skipping, section-aware) so this check
	// can never pass or fail based on a commented-out backup line.
	FString ArtifactsLine;
	if (!FEOSArtifactsParser::FindLiveArtifactsLine(IniText, ArtifactsLine))
	{
		OutDiagnostics.Emplace(EMessageSeverity::Error,
			TEXT("No 'Artifacts=' line found in DefaultEngine.ini. EOS credentials are missing. Add your Product ID, Sandbox ID, Deployment ID, Client ID, and Client Secret — the Setup tab will write them for you."));
		return false;
	}

	// Check for YOUR_ placeholders
	if (ArtifactsLine.Contains(TEXT("YOUR_")))
	{
		OutDiagnostics.Emplace(EMessageSeverity::Error,
			TEXT("EOS credentials still contain placeholder values (YOUR_...). Replace them with your actual credentials from the EOS Developer Portal."));
		return false;
	}

	// Check for empty credential fields
	bool bPassed = true;
	const TArray<FString> RequiredFields = { TEXT("ClientId"), TEXT("ClientSecret"), TEXT("ProductId"), TEXT("SandboxId"), TEXT("DeploymentId") };
	for (const FString& Field : RequiredFields)
	{
		FString Pattern = Field + TEXT("=\"\"");
		if (ArtifactsLine.Contains(Pattern))
		{
			OutDiagnostics.Emplace(EMessageSeverity::Error,
				FString::Printf(TEXT("EOS credential '%s' is empty in Artifacts line. Set it to your value from the EOS Developer Portal."), *Field));
			bPassed = false;
		}
	}

	return bPassed;
}

// #2: DefaultArtifactName must point at the Artifacts entry, or credential lookup fails
//
// At startup the engine resolves DefaultArtifactName (engine default: empty) against the
// Artifacts array, with a last-chance fallback to an entry whose ArtifactName is empty.
// A named entry with no matching DefaultArtifactName is therefore never found — EOS gets
// blank platform options and everything downstream silently fails.
bool FEOSConfigValidator::CheckArtifactNameMatch(const FString& IniText, const FString& CleanIni, TArray<FEOSConfigDiagnostic>& OutDiagnostics)
{
	FString ArtifactsLine;
	if (!FEOSArtifactsParser::FindLiveArtifactsLine(IniText, ArtifactsLine))
	{
		return true; // no credentials at all — CheckCredentials already reports that
	}

	const FString ArtifactName = FEOSArtifactsParser::ExtractField(ArtifactsLine, TEXT("ArtifactName"));
	if (ArtifactName.IsEmpty())
	{
		return true; // empty-name entries are matched by the engine's last-chance fallback
	}

	FString DefaultName;
	const FString Key = TEXT("DefaultArtifactName=");
	const int32 KeyIndex = CleanIni.Find(Key, ESearchCase::IgnoreCase);
	if (KeyIndex != INDEX_NONE)
	{
		const int32 ValueStart = KeyIndex + Key.Len();
		const int32 LineEnd = CleanIni.Find(TEXT("\n"), ESearchCase::CaseSensitive, ESearchDir::FromStart, ValueStart);
		DefaultName = (LineEnd != INDEX_NONE)
			? CleanIni.Mid(ValueStart, LineEnd - ValueStart)
			: CleanIni.Mid(ValueStart);
		DefaultName = DefaultName.TrimStartAndEnd().TrimQuotes();
	}

	if (!DefaultName.Equals(ArtifactName, ESearchCase::IgnoreCase))
	{
		OutDiagnostics.Emplace(EMessageSeverity::Error,
			FString::Printf(TEXT("Artifacts entry is named '%s' but DefaultArtifactName is %s. EOS resolves credentials by this name at startup and would find nothing. Add 'DefaultArtifactName=%s' under [/Script/OnlineSubsystemEOS.EOSSettings] — the Setup tab's Save adds it when missing."),
				*ArtifactName,
				DefaultName.IsEmpty() ? TEXT("not set") : *FString::Printf(TEXT("'%s'"), *DefaultName),
				*ArtifactName));
		return false;
	}

	return true;
}

// #3: Wrong ini section name (bare [EOSSettings] without full path)
bool FEOSConfigValidator::CheckSectionName(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics)
{
	// Line scan: a section header is a whole line, so the error case is a line that IS
	// '[EOSSettings]' after trimming. The correct full header can never produce such a
	// line, which is why no substring/lookback dance is needed here.
	TArray<FString> Lines;
	IniText.ParseIntoArrayLines(Lines, false);

	for (const FString& Line : Lines)
	{
		if (Line.TrimStartAndEnd().Equals(TEXT("[EOSSettings]"), ESearchCase::IgnoreCase))
		{
			OutDiagnostics.Emplace(EMessageSeverity::Error,
				TEXT("Found bare '[EOSSettings]' section in DefaultEngine.ini. It must be '[/Script/OnlineSubsystemEOS.EOSSettings]' (full path with /Script/ prefix)."));
			return false;
		}
	}

	return true;
}

// #4: AuthScopeFlags present (causes EOSShared.dll crash)
bool FEOSConfigValidator::CheckAuthScopeFlags(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics)
{
	if (IniText.Contains(TEXT("AuthScopeFlags"), ESearchCase::IgnoreCase))
	{
		OutDiagnostics.Emplace(EMessageSeverity::Error,
			TEXT("'AuthScopeFlags' found in DefaultEngine.ini. Remove it — this causes a crash in EOSShared.dll on startup."));
		return false;
	}

	return true;
}

// #5: Missing DefaultPlatformService=EOS
bool FEOSConfigValidator::CheckDefaultPlatformService(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics)
{
	// Look for [OnlineSubsystem] section with DefaultPlatformService=EOS
	const bool bHasSection = IniText.Contains(TEXT("[OnlineSubsystem]"), ESearchCase::IgnoreCase);
	const bool bHasEOS = IniText.Contains(TEXT("DefaultPlatformService=EOS"), ESearchCase::IgnoreCase);

	if (!bHasSection || !bHasEOS)
	{
		OutDiagnostics.Emplace(EMessageSeverity::Error,
			TEXT("Missing '[OnlineSubsystem] DefaultPlatformService=EOS' in DefaultEngine.ini. Without this, the engine won't use EOS as the online subsystem."));
		return false;
	}

	return true;
}

// #6 & #7: Plugin enabled checks
//
// Asks IPluginManager rather than parsing the .uproject: a plugin can be enabled transitively
// (this kit's own .uplugin enables both EOS plugins in its "Plugins" array), in which case it is
// genuinely on but never appears in the .uproject text.
bool FEOSConfigValidator::CheckPluginEnabled(const FString& PluginName, TArray<FEOSConfigDiagnostic>& OutDiagnostics)
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);

	if (!Plugin.IsValid())
	{
		OutDiagnostics.Emplace(EMessageSeverity::Error,
			FString::Printf(TEXT("Plugin '%s' was not found in this engine or project. It ships with Unreal Engine — check your engine installation."), *PluginName));
		return false;
	}

	if (!Plugin->IsEnabled())
	{
		OutDiagnostics.Emplace(EMessageSeverity::Error,
			FString::Printf(TEXT("Plugin '%s' is installed but disabled. Enable it in Edit > Plugins, or add it to your .uproject Plugins array with \"Enabled\": true."), *PluginName));
		return false;
	}

	return true;
}

// #8: Missing or ineffective NetDriverEOS config
bool FEOSConfigValidator::CheckNetDriverConfig(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics)
{
	const bool bHasGameEngine = IniText.Contains(TEXT("[/Script/Engine.GameEngine]"), ESearchCase::IgnoreCase);
	const bool bHasNetDriverEOS = IniText.Contains(TEXT("NetDriverEOS"), ESearchCase::IgnoreCase);

	if (!bHasGameEngine || !bHasNetDriverEOS)
	{
		OutDiagnostics.Emplace(EMessageSeverity::Warning,
			TEXT("Missing NetDriverEOS configuration under '[/Script/Engine.GameEngine]' in DefaultEngine.ini. P2P connections may fail without the EOS net driver."));
		return false;
	}

	// Present but ineffective: without clearing the array first, the engine's built-in
	// GameNetDriver (IpNetDriver) definition from BaseEngine.ini comes first and wins.
	if (!IniText.Contains(TEXT("!NetDriverDefinitions="), ESearchCase::IgnoreCase))
	{
		OutDiagnostics.Emplace(EMessageSeverity::Warning,
			TEXT("NetDriverEOS is configured but '!NetDriverDefinitions=ClearArray' is missing. The engine's built-in GameNetDriver definition comes first in the array and wins, so the EOS net driver would silently never be used. Add the ClearArray line above your +NetDriverDefinitions entry."));
		return false;
	}

	return true;
}

// #9: bUseEAS / bUseEOSConnect (engine defaults: false)
bool FEOSConfigValidator::CheckAuthFlags(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics)
{
	const bool bUseEAS = IniText.Contains(TEXT("bUseEAS=true"), ESearchCase::IgnoreCase);
	const bool bUseConnect = IniText.Contains(TEXT("bUseEOSConnect=true"), ESearchCase::IgnoreCase);

	if (!bUseEAS || !bUseConnect)
	{
		const TCHAR* Missing = (!bUseEAS && !bUseConnect)
			? TEXT("'bUseEAS=true' and 'bUseEOSConnect=true'")
			: (!bUseEAS ? TEXT("'bUseEAS=true'") : TEXT("'bUseEOSConnect=true'"));

		OutDiagnostics.Emplace(EMessageSeverity::Warning,
			FString::Printf(TEXT("Missing %s under [/Script/OnlineSubsystemEOS.EOSSettings] — the engine defaults both to false. Epic Account login and EOS Connect features (sessions, presence) rely on them."), Missing));
		return false;
	}

	return true;
}

// #10: Overlay flags (engine defaults: false)
bool FEOSConfigValidator::CheckOverlayFlags(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics)
{
	const bool bOverlay = IniText.Contains(TEXT("bEnableOverlay=true"), ESearchCase::IgnoreCase);
	const bool bSocial = IniText.Contains(TEXT("bEnableSocialOverlay=true"), ESearchCase::IgnoreCase);

	if (!bOverlay || !bSocial)
	{
		OutDiagnostics.Emplace(EMessageSeverity::Warning,
			TEXT("Epic overlay is not enabled — launched clients won't show the EOS overlay or invite UI. Add 'bEnableOverlay=true' and 'bEnableSocialOverlay=true' under [/Script/OnlineSubsystemEOS.EOSSettings] (engine defaults are false). Keep 'bEnableEditorOverlay=false'."));
		return false;
	}

	return true;
}

// #11: bEnableEditorOverlay=true causes PIE crashes
bool FEOSConfigValidator::CheckEditorOverlay(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics)
{
	if (IniText.Contains(TEXT("bEnableEditorOverlay=true"), ESearchCase::IgnoreCase))
	{
		OutDiagnostics.Emplace(EMessageSeverity::Warning,
			TEXT("'bEnableEditorOverlay=true' found in DefaultEngine.ini. This causes PIE crashes. Set it to false — the EOS Overlay is for packaged builds only."));
		return false;
	}

	return true;
}

// #12: EncryptionKey without Client prefix
bool FEOSConfigValidator::CheckEncryptionKey(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics)
{
	static const FString Key = TEXT("EncryptionKey");
	static const FString Prefix = TEXT("Client");

	int32 SearchStart = 0;
	while (true)
	{
		int32 FoundIndex = IniText.Find(Key, ESearchCase::IgnoreCase, ESearchDir::FromStart, SearchStart);
		if (FoundIndex == INDEX_NONE)
		{
			break;
		}

		const bool bHasClientPrefix =
			FoundIndex >= Prefix.Len() &&
			IniText.Mid(FoundIndex - Prefix.Len(), Prefix.Len()).Equals(Prefix, ESearchCase::IgnoreCase);

		if (!bHasClientPrefix)
		{
			OutDiagnostics.Emplace(EMessageSeverity::Warning,
				TEXT("Found 'EncryptionKey' without 'Client' prefix in DefaultEngine.ini. In UE 5.7, use 'ClientEncryptionKey' instead of 'EncryptionKey'."));
			return false;
		}

		SearchStart = FoundIndex + Key.Len();
	}

	return true;
}
