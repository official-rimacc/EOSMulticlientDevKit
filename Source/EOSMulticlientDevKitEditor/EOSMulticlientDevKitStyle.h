// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Slate style (plugin icon brushes)

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

/**
 * Registers the plugin's icon (Resources/Icon128.png) as Slate brushes so the
 * control panel tab, toolbar button, and Tools menu entry carry the plugin's
 * own icon instead of the engine's stock gear.
 */
class FEOSMulticlientDevKitStyle
{
public:
	static void Initialize();
	static void Shutdown();

	static const ISlateStyle& Get();
	static FName GetStyleSetName();

private:
	static TSharedPtr<FSlateStyleSet> StyleInstance;
};
