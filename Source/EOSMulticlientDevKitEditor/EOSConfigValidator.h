// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — EOS Configuration Validator

#pragma once

#include "CoreMinimal.h"
#include "Logging/MessageLog.h"

/**
 * Diagnostic result from a single validation check.
 */
struct FEOSConfigDiagnostic
{
	EMessageSeverity::Type Severity;
	FString Message;

	FEOSConfigDiagnostic(EMessageSeverity::Type InSeverity, const FString& InMessage)
		: Severity(InSeverity), Message(InMessage)
	{
	}
};

/**
 * How many checks ran and how many were clean.
 *
 * Tracked separately from the diagnostic count because a single check can emit several
 * diagnostics (CheckCredentials reports one per empty credential field), so
 * "checks - diagnostics" is not a valid pass count.
 */
struct FEOSValidationSummary
{
	int32 TotalChecks = 0;
	int32 PassedChecks = 0;
};

/**
 * FEOSConfigValidator
 *
 * Static utility class that validates EOS configuration by reading raw ini text
 * (not GConfig, which silently merges bad section names) and querying IPluginManager.
 *
 * Returns diagnostics via an output array — callers decide how to present them
 * (Message Log, toast notifications, etc.).
 */
class FEOSConfigValidator
{
public:
	/** Run all validation checks, append results to OutDiagnostics, and return the pass tally. */
	static FEOSValidationSummary Validate(TArray<FEOSConfigDiagnostic>& OutDiagnostics);

private:
	/** Each check returns true when it found no problem. */
	static bool CheckCredentials(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics);
	static bool CheckArtifactNameMatch(const FString& IniText, const FString& CleanIni, TArray<FEOSConfigDiagnostic>& OutDiagnostics);
	static bool CheckSectionName(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics);
	static bool CheckAuthScopeFlags(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics);
	static bool CheckDefaultPlatformService(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics);
	static bool CheckPluginEnabled(const FString& PluginName, TArray<FEOSConfigDiagnostic>& OutDiagnostics);
	static bool CheckNetDriverConfig(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics);
	static bool CheckAuthFlags(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics);
	static bool CheckOverlayFlags(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics);
	static bool CheckEditorOverlay(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics);
	static bool CheckEncryptionKey(const FString& IniText, TArray<FEOSConfigDiagnostic>& OutDiagnostics);
};
