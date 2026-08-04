// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.
// EOSMulticlientDevKit — Validate Tab

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SVerticalBox;

class SEOSValidateTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEOSValidateTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedPtr<SVerticalBox> ResultsBox;

	FReply OnRevalidateClicked();
	void RunValidation();
};
