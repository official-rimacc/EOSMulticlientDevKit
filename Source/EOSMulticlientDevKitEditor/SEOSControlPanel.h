// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Control Panel Root Widget

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SEOSControlPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEOSControlPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	int32 ActiveTabIndex = 0;

	FReply OnTabClicked(int32 TabIndex);
	int32 GetActiveTabIndex() const { return ActiveTabIndex; }
	FSlateColor GetTabButtonColor(int32 TabIndex) const;
	FSlateFontInfo GetTabButtonFont(int32 TabIndex) const;
};
