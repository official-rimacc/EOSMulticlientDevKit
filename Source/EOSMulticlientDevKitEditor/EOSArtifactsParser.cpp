// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Artifacts Line Parser

#include "EOSArtifactsParser.h"
#include "EOSConfigTextUtils.h"
#include "EOSMulticlientDevKitEditorLog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	/** True if this trimmed, non-comment line is an Artifacts= entry ('+Artifacts=' included). */
	bool IsArtifactsLine(const FString& TrimmedLine)
	{
		const FString Key = TrimmedLine.StartsWith(TEXT("+")) ? TrimmedLine.RightChop(1) : TrimmedLine;
		return Key.StartsWith(TEXT("Artifacts="), ESearchCase::IgnoreCase);
	}
}

FString FEOSArtifactsParser::GetIniPath()
{
	return FPaths::ProjectConfigDir() / TEXT("DefaultEngine.ini");
}

int32 FEOSArtifactsParser::FindLiveArtifactsLineIndex(const TArray<FString>& Lines)
{
	int32 FallbackIndex = INDEX_NONE;
	bool bInEOSSettingsSection = false;

	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		const FString Trimmed = Lines[Index].TrimStartAndEnd();

		if (Trimmed.IsEmpty() || IsCommentLine(Trimmed))
		{
			continue;
		}

		if (Trimmed.StartsWith(TEXT("[")))
		{
			bInEOSSettingsSection = Trimmed.Equals(EOSSettingsSectionHeader, ESearchCase::IgnoreCase);
			continue;
		}

		if (IsArtifactsLine(Trimmed))
		{
			if (bInEOSSettingsSection)
			{
				return Index; // live line in the right section — best possible match
			}
			if (FallbackIndex == INDEX_NONE)
			{
				FallbackIndex = Index; // live line in another section; remember the first
			}
		}
	}

	return FallbackIndex;
}

bool FEOSArtifactsParser::FindLiveArtifactsLine(const FString& IniText, FString& OutLine)
{
	TArray<FString> Lines;
	IniText.ParseIntoArrayLines(Lines, false);

	const int32 LineIndex = FindLiveArtifactsLineIndex(Lines);
	if (LineIndex == INDEX_NONE)
	{
		return false;
	}

	OutLine = Lines[LineIndex];
	return true;
}

FString FEOSArtifactsParser::ExtractField(const FString& ArtifactsLine, const FString& FieldName)
{
	FString Pattern = FieldName + TEXT("=\"");
	int32 Start = ArtifactsLine.Find(Pattern, ESearchCase::IgnoreCase);
	if (Start == INDEX_NONE)
	{
		return FString();
	}

	Start += Pattern.Len();
	int32 End = ArtifactsLine.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, Start);
	if (End == INDEX_NONE)
	{
		return FString();
	}

	return ArtifactsLine.Mid(Start, End - Start);
}

bool FEOSArtifactsParser::Load(FEOSArtifactCredentials& OutCredentials)
{
	FString IniText;
	if (!FFileHelper::LoadFileToString(IniText, *GetIniPath()))
	{
		UE_LOG(LogEOSMulticlientDevKit, Warning, TEXT("FEOSArtifactsParser: Could not read DefaultEngine.ini"));
		return false;
	}

	FString ArtifactsLine;
	if (!FindLiveArtifactsLine(IniText, ArtifactsLine))
	{
		UE_LOG(LogEOSMulticlientDevKit, Warning, TEXT("FEOSArtifactsParser: No live Artifacts= line found"));
		return false;
	}

	OutCredentials.ArtifactName = ExtractField(ArtifactsLine, TEXT("ArtifactName"));
	OutCredentials.ClientId = ExtractField(ArtifactsLine, TEXT("ClientId"));
	OutCredentials.ClientSecret = ExtractField(ArtifactsLine, TEXT("ClientSecret"));
	OutCredentials.ProductId = ExtractField(ArtifactsLine, TEXT("ProductId"));
	OutCredentials.SandboxId = ExtractField(ArtifactsLine, TEXT("SandboxId"));
	OutCredentials.DeploymentId = ExtractField(ArtifactsLine, TEXT("DeploymentId"));
	OutCredentials.ClientEncryptionKey = ExtractField(ArtifactsLine, TEXT("ClientEncryptionKey"));

	return true;
}

FString FEOSArtifactsParser::BuildArtifactsLine(const FEOSArtifactCredentials& Credentials)
{
	return FString::Printf(
		TEXT("Artifacts=(ArtifactName=\"%s\",ClientId=\"%s\",ClientSecret=\"%s\",ProductId=\"%s\",SandboxId=\"%s\",DeploymentId=\"%s\",ClientEncryptionKey=\"%s\")"),
		*Credentials.ArtifactName,
		*Credentials.ClientId,
		*Credentials.ClientSecret,
		*Credentials.ProductId,
		*Credentials.SandboxId,
		*Credentials.DeploymentId,
		*Credentials.ClientEncryptionKey
	);
}

bool FEOSArtifactsParser::Save(const FEOSArtifactCredentials& Credentials)
{
	const FString IniPath = GetIniPath();
	FString IniText;
	if (!FFileHelper::LoadFileToString(IniText, *IniPath))
	{
		UE_LOG(LogEOSMulticlientDevKit, Error, TEXT("FEOSArtifactsParser: Could not read DefaultEngine.ini for save"));
		return false;
	}

	// Preserve the file's dominant line ending so a save doesn't rewrite every line in VCS.
	const TCHAR* LineEnding = IniText.Contains(TEXT("\r\n")) ? TEXT("\r\n") : TEXT("\n");

	TArray<FString> Lines;
	IniText.ParseIntoArrayLines(Lines, false);

	const FString NewArtifactsLine = BuildArtifactsLine(Credentials);
	const int32 ExistingIndex = FindLiveArtifactsLineIndex(Lines);

	if (ExistingIndex != INDEX_NONE)
	{
		// Replace the live line, preserving a '+' array-append prefix if it had one.
		const bool bHadPlus = Lines[ExistingIndex].TrimStartAndEnd().StartsWith(TEXT("+"));
		Lines[ExistingIndex] = bHadPlus ? FString(TEXT("+")) + NewArtifactsLine : NewArtifactsLine;
	}
	else
	{
		// No live Artifacts= line — insert one after [/Script/OnlineSubsystemEOS.EOSSettings],
		// creating the section at the end of the file if it doesn't exist yet.
		int32 SectionIndex = INDEX_NONE;
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			if (Lines[Index].TrimStartAndEnd().Equals(EOSSettingsSectionHeader, ESearchCase::IgnoreCase))
			{
				SectionIndex = Index;
				break;
			}
		}

		if (SectionIndex == INDEX_NONE)
		{
			Lines.Add(FString());
			Lines.Add(EOSSettingsSectionHeader);
			SectionIndex = Lines.Num() - 1;
		}

		// '+' = append to the Artifacts config array, matching the engine's documented form.
		Lines.Insert(FString(TEXT("+")) + NewArtifactsLine, SectionIndex + 1);
	}

	const FString OutText = FString::Join(Lines, LineEnding);
	if (!FFileHelper::SaveStringToFile(OutText, *IniPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogEOSMulticlientDevKit, Error, TEXT("FEOSArtifactsParser: Failed to write DefaultEngine.ini"));
		return false;
	}

	UE_LOG(LogEOSMulticlientDevKit, Log, TEXT("FEOSArtifactsParser: Saved credentials to DefaultEngine.ini"));
	return true;
}
