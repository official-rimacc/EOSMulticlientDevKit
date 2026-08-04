// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Recommended Config Writer

#pragma once

#include "CoreMinimal.h"

/**
 * FEOSConfigWriter
 *
 * Writes the non-credential EOS config block into DefaultEngine.ini:
 * DefaultArtifactName, auth + overlay flags, DefaultPlatformService, and the
 * EOS net driver definitions — everything the validator checks for that the
 * Artifacts= line alone doesn't cover.
 *
 * Add-only by design: a missing line is added with the recommended value, but an
 * existing line is never rewritten, even when its value differs — deliberate
 * project choices stay intact, and FEOSConfigValidator reports conflicts instead.
 *
 * Uses the same raw-text, comment-aware line handling as FEOSArtifactsParser.
 */
class FEOSConfigWriter
{
public:
	/**
	 * Ensure the recommended EOS config exists in DefaultEngine.ini.
	 *
	 * ArtifactName seeds DefaultArtifactName so the engine's credential lookup
	 * matches the Artifacts entry the Setup tab writes.
	 *
	 * Appends a human-readable entry to OutAppliedChanges for every line added.
	 * Returns false only when the ini could not be read or written.
	 */
	static bool ApplyRecommendedConfig(const FString& ArtifactName, TArray<FString>& OutAppliedChanges);
};
