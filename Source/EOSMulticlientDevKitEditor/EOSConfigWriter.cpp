// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Recommended Config Writer

#include "EOSConfigWriter.h"
#include "EOSConfigTextUtils.h"
#include "EOSMulticlientDevKitEditorLog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	const TCHAR* GameEngineSectionHeader = TEXT("[/Script/Engine.GameEngine]");

	/** Line-array view of an ini file with comment-aware, section-scoped operations. */
	struct FIniLines
	{
		TArray<FString> Lines;

		int32 FindSection(const FString& Header) const
		{
			for (int32 Index = 0; Index < Lines.Num(); ++Index)
			{
				const FString Trimmed = Lines[Index].TrimStartAndEnd();
				if (!IsCommentLine(Trimmed) && Trimmed.Equals(Header, ESearchCase::IgnoreCase))
				{
					return Index;
				}
			}
			return INDEX_NONE;
		}

		int32 FindOrAddSection(const FString& Header, TArray<FString>& OutAppliedChanges)
		{
			const int32 Found = FindSection(Header);
			if (Found != INDEX_NONE)
			{
				return Found;
			}

			if (Lines.Num() > 0 && !Lines.Last().TrimStartAndEnd().IsEmpty())
			{
				Lines.Add(FString());
			}
			Lines.Add(Header);
			OutAppliedChanges.Add(FString::Printf(TEXT("added section %s"), *Header));
			return Lines.Num() - 1;
		}

		/** Index one past the last line of the section (i.e. the next section header, or EOF). */
		int32 SectionEnd(int32 SectionIndex) const
		{
			for (int32 Index = SectionIndex + 1; Index < Lines.Num(); ++Index)
			{
				const FString Trimmed = Lines[Index].TrimStartAndEnd();
				if (!Trimmed.IsEmpty() && !IsCommentLine(Trimmed) && Trimmed.StartsWith(TEXT("[")))
				{
					return Index;
				}
			}
			return Lines.Num();
		}

		/** Find the live line in this section whose trimmed form starts with Prefix (and, if given, contains RequiredSubstring). */
		int32 FindLineInSection(int32 SectionIndex, const FString& Prefix, const FString& RequiredSubstring = FString()) const
		{
			const int32 End = SectionEnd(SectionIndex);
			for (int32 Index = SectionIndex + 1; Index < End; ++Index)
			{
				const FString Trimmed = Lines[Index].TrimStartAndEnd();
				if (Trimmed.IsEmpty() || IsCommentLine(Trimmed))
				{
					continue;
				}
				if (Trimmed.StartsWith(Prefix, ESearchCase::IgnoreCase) &&
					(RequiredSubstring.IsEmpty() || Trimmed.Contains(RequiredSubstring, ESearchCase::IgnoreCase)))
				{
					return Index;
				}
			}
			return INDEX_NONE;
		}

		/** Insert at the end of the section, before any blank separator lines. */
		void InsertAtSectionEnd(int32 SectionIndex, const FString& NewLine)
		{
			int32 InsertIndex = SectionEnd(SectionIndex);
			while (InsertIndex > SectionIndex + 1 && Lines[InsertIndex - 1].TrimStartAndEnd().IsEmpty())
			{
				--InsertIndex;
			}
			Lines.Insert(NewLine, InsertIndex);
		}

		/** Add DesiredLine to the section if no live line for KeyPrefix exists. Never rewrites an existing line. */
		void EnsureLine(const FString& SectionHeader, const FString& KeyPrefix, const FString& DesiredLine, TArray<FString>& OutAppliedChanges)
		{
			const int32 SectionIndex = FindOrAddSection(SectionHeader, OutAppliedChanges);
			if (FindLineInSection(SectionIndex, KeyPrefix) == INDEX_NONE)
			{
				InsertAtSectionEnd(SectionIndex, DesiredLine);
				OutAppliedChanges.Add(DesiredLine);
			}
		}
	};
}

bool FEOSConfigWriter::ApplyRecommendedConfig(const FString& ArtifactName, TArray<FString>& OutAppliedChanges)
{
	const FString IniPath = FPaths::ProjectConfigDir() / TEXT("DefaultEngine.ini");
	FString IniText;
	if (!FFileHelper::LoadFileToString(IniText, *IniPath))
	{
		UE_LOG(LogEOSMulticlientDevKit, Error, TEXT("FEOSConfigWriter: Could not read DefaultEngine.ini"));
		return false;
	}

	// Preserve the file's dominant line ending so a save doesn't rewrite every line in VCS.
	const TCHAR* LineEnding = IniText.Contains(TEXT("\r\n")) ? TEXT("\r\n") : TEXT("\n");

	FIniLines Ini;
	IniText.ParseIntoArrayLines(Ini.Lines, false);

	// [OnlineSubsystem] — route the engine's online layer to EOS.
	Ini.EnsureLine(TEXT("[OnlineSubsystem]"), TEXT("DefaultPlatformService="), TEXT("DefaultPlatformService=EOS"), OutAppliedChanges);

	// [OnlineSubsystemEOS] — explicit enable, matching Epic's documented setup.
	Ini.EnsureLine(TEXT("[OnlineSubsystemEOS]"), TEXT("bEnabled="), TEXT("bEnabled=true"), OutAppliedChanges);

	// [.EOSSettings] — artifact selection plus the flags whose engine defaults (false)
	// disable exactly what this kit exists for (auto-login, sessions, per-client overlay).
	Ini.EnsureLine(EOSSettingsSectionHeader, TEXT("DefaultArtifactName="), FString::Printf(TEXT("DefaultArtifactName=%s"), *ArtifactName), OutAppliedChanges);
	Ini.EnsureLine(EOSSettingsSectionHeader, TEXT("bUseEAS="), TEXT("bUseEAS=true"), OutAppliedChanges);
	Ini.EnsureLine(EOSSettingsSectionHeader, TEXT("bUseEOSConnect="), TEXT("bUseEOSConnect=true"), OutAppliedChanges);
	Ini.EnsureLine(EOSSettingsSectionHeader, TEXT("bEnableOverlay="), TEXT("bEnableOverlay=true"), OutAppliedChanges);
	Ini.EnsureLine(EOSSettingsSectionHeader, TEXT("bEnableSocialOverlay="), TEXT("bEnableSocialOverlay=true"), OutAppliedChanges);
	Ini.EnsureLine(EOSSettingsSectionHeader, TEXT("bEnableEditorOverlay="), TEXT("bEnableEditorOverlay=false"), OutAppliedChanges);

	// [/Script/Engine.GameEngine] — EOS net driver. Order matters: without the ClearArray
	// line, the engine's built-in GameNetDriver (IpNetDriver) definition from BaseEngine.ini
	// comes first in the array and wins, so the EOS driver would silently never be used.
	{
		const int32 SectionIndex = Ini.FindOrAddSection(GameEngineSectionHeader, OutAppliedChanges);
		const bool bHasGameNetDriver =
			Ini.FindLineInSection(SectionIndex, TEXT("+NetDriverDefinitions="), TEXT("DefName=\"GameNetDriver\"")) != INDEX_NONE;

		if (Ini.FindLineInSection(SectionIndex, TEXT("!NetDriverDefinitions=")) == INDEX_NONE)
		{
			// Top of section: clears the engine defaults while every '+' line in this file,
			// coming after it, stays effective.
			Ini.Lines.Insert(TEXT("!NetDriverDefinitions=ClearArray"), SectionIndex + 1);
			OutAppliedChanges.Add(TEXT("!NetDriverDefinitions=ClearArray"));
		}

		if (!bHasGameNetDriver)
		{
			const FString DriverLine = TEXT("+NetDriverDefinitions=(DefName=\"GameNetDriver\",DriverClassName=\"/Script/SocketSubsystemEOS.NetDriverEOS\",DriverClassNameFallback=\"/Script/OnlineSubsystemUtils.IpNetDriver\")");
			Ini.InsertAtSectionEnd(SectionIndex, DriverLine);
			OutAppliedChanges.Add(DriverLine);
		}
	}

	if (OutAppliedChanges.Num() == 0)
	{
		return true; // everything already present — nothing written
	}

	const FString OutText = FString::Join(Ini.Lines, LineEnding);
	if (!FFileHelper::SaveStringToFile(OutText, *IniPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogEOSMulticlientDevKit, Error, TEXT("FEOSConfigWriter: Failed to write DefaultEngine.ini"));
		return false;
	}

	for (const FString& Change : OutAppliedChanges)
	{
		UE_LOG(LogEOSMulticlientDevKit, Log, TEXT("FEOSConfigWriter: %s"), *Change);
	}
	return true;
}
