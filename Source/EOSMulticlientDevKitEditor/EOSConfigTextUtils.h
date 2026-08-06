// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Shared Config Text
//
// Single home for the ini strings and line predicates used by more than one .cpp
// in this module. These previously had a private copy per file. That does not
// survive a unity build: UBT concatenates several .cpp files into one translation
// unit, anonymous namespaces from the constituent files merge into one, and the
// copies collide. Adaptive unity hides it in any working copy where the files have
// been touched, so it only shows up on a clean build of an unmodified tree.

#pragma once

#include "CoreMinimal.h"

/** Section header for the EOS settings block in DefaultEngine.ini. */
inline constexpr const TCHAR* EOSSettingsSectionHeader = TEXT("[/Script/OnlineSubsystemEOS.EOSSettings]");

/** Section holding the Dev Auth Tool port and per-client credential names. */
inline constexpr const TCHAR* DevAuthSection = TEXT("EOSMulticlientDevKit.DevAuthTool");

/** True if this already-trimmed line is a whole-line comment (';' / '#'). */
inline bool IsCommentLine(const FString& TrimmedLine)
{
	return TrimmedLine.StartsWith(TEXT(";")) || TrimmedLine.StartsWith(TEXT("#"));
}
