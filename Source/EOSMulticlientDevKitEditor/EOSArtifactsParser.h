// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Artifacts Line Parser

#pragma once

#include "CoreMinimal.h"

/**
 * Parsed EOS credentials from the Artifacts= line in DefaultEngine.ini.
 */
struct FEOSArtifactCredentials
{
	FString ArtifactName;
	FString ClientId;
	FString ClientSecret;
	FString ProductId;
	FString SandboxId;
	FString DeploymentId;
	FString ClientEncryptionKey;
};

/**
 * FEOSArtifactsParser
 *
 * Reads and writes the Artifacts= line in DefaultEngine.ini using raw file I/O.
 * GConfig doesn't handle the parenthesized struct syntax reliably, so we match
 * the same raw-text approach used by FEOSConfigValidator.
 *
 * All lookups are line-based: comment lines (';' / '#') are skipped, and a match
 * inside [/Script/OnlineSubsystemEOS.EOSSettings] is preferred over one in any
 * other section — a commented-out backup line must never shadow the live one.
 */
class FEOSArtifactsParser
{
public:
	/** Load credentials from the live Artifacts= line in DefaultEngine.ini. */
	static bool Load(FEOSArtifactCredentials& OutCredentials);

	/** Save credentials back to the live Artifacts= line in DefaultEngine.ini. */
	static bool Save(const FEOSArtifactCredentials& Credentials);

	/**
	 * Find the live (non-comment) Artifacts= line in raw ini text.
	 * Shared with FEOSConfigValidator so validation and load/save can never
	 * disagree about which line is the live one.
	 */
	static bool FindLiveArtifactsLine(const FString& IniText, FString& OutLine);

	/** Extract a quoted field value (Field="...") from an Artifacts line. Empty if absent. */
	static FString ExtractField(const FString& ArtifactsLine, const FString& FieldName);

private:
	static FString GetIniPath();
	static FString BuildArtifactsLine(const FEOSArtifactCredentials& Credentials);
	static int32 FindLiveArtifactsLineIndex(const TArray<FString>& Lines);
};
