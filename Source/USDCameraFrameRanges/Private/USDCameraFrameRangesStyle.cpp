// Copyright Epic Games, Inc. All Rights Reserved.

#include "USDCameraFrameRangesStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FUSDCameraFrameRangesStyle::StyleInstance = nullptr;

void FUSDCameraFrameRangesStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FUSDCameraFrameRangesStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FUSDCameraFrameRangesStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("USDCameraFrameRangesStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FUSDCameraFrameRangesStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("USDCameraFrameRangesStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("USDCameraFrameRanges")->GetBaseDir() / TEXT("Resources"));

	Style->Set("USDCameraFrameRanges.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));

	return Style;
}

void FUSDCameraFrameRangesStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FUSDCameraFrameRangesStyle::Get()
{
	return *StyleInstance;
}
