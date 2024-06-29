// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "USDCameraFrameRangesStyle.h"

class FUSDCameraFrameRangesCommands : public TCommands<FUSDCameraFrameRangesCommands>
{
public:

	FUSDCameraFrameRangesCommands()
		: TCommands<FUSDCameraFrameRangesCommands>(TEXT("USDCameraFrameRanges"), NSLOCTEXT("Contexts", "USDCameraFrameRanges", "USDCameraFrameRanges Plugin"), NAME_None, FUSDCameraFrameRangesStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};