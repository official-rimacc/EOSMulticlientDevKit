// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Slate style (plugin icon brushes)

#include "EOSMulticlientDevKitStyle.h"
#include "Brushes/SlateImageBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FEOSMulticlientDevKitStyle::StyleInstance = nullptr;

void FEOSMulticlientDevKitStyle::Initialize()
{
	if (StyleInstance.IsValid())
	{
		return;
	}

	StyleInstance = MakeShared<FSlateStyleSet>(GetStyleSetName());

	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("EOSMulticlientDevKit"));
	if (Plugin.IsValid())
	{
		StyleInstance->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources"));
		const FString IconPath = StyleInstance->RootToContentDir(TEXT("Icon128"), TEXT(".png"));

		// One brush per consumer size: tabs/menus draw 16, the toolbar draws 20.
		StyleInstance->Set("EOSMulticlientDevKit.Icon", new FSlateImageBrush(IconPath, FVector2D(16.0, 16.0)));
		StyleInstance->Set("EOSMulticlientDevKit.Icon.Toolbar", new FSlateImageBrush(IconPath, FVector2D(20.0, 20.0)));
	}

	FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

void FEOSMulticlientDevKitStyle::Shutdown()
{
	if (StyleInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
		StyleInstance.Reset();
	}
}

const ISlateStyle& FEOSMulticlientDevKitStyle::Get()
{
	return *StyleInstance;
}

FName FEOSMulticlientDevKitStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("EOSMulticlientDevKitStyle"));
	return StyleSetName;
}
