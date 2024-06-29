// Copyright Epic Games, Inc. All Rights Reserved.

#include "USDCameraFrameRangesCommands.h"

#define LOCTEXT_NAMESPACE "FUSDCameraFrameRangesModule"

void FUSDCameraFrameRangesCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "USDCameraFrameRanges", "Bring up USDCameraFrameRanges window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
