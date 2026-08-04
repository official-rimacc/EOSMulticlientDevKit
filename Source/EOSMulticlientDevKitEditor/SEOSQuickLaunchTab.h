// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Quick Launch Tab

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SEOSQuickLaunchTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEOSQuickLaunchTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnLaunchStandalone(int32 NumPlayers);

	/** bAsGuest launches the instance with -EosGuestLogin (anonymous Device ID login) instead of a
	 *  DevAuthTool credential — no DevAuthTool or Epic account needed for that instance. */
	void LaunchStandaloneProcess(const FString& Credential, bool bAsGuest, int32 WinX, int32 WinY, int32 ResX, int32 ResY);

	/** When true, the LAST launched client logs in as an anonymous Device ID guest. Only one guest
	 *  per machine: the device id is per-Windows-user, two guests would share the same PUID. */
	bool bLastClientIsGuest = false;
};
