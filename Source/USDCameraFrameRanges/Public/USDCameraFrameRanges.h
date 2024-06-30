// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"


class ACineCameraActor;

namespace UE
{
	class FSdfPath;
	class FUsdPrim;
}

class FToolBarBuilder;
class FMenuBuilder;
class AUsdStageActor;

struct FCameraInfo
{
	FString CameraName;
	int32 StartFrame;
	int32 EndFrame;
};

class FUSDCameraFrameRangesModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	
private:

	void RegisterMenus();

	// TArray<FCameraInfo> GetCamerasFromUSDStage(TObjectPtr<AUsdStageActor> USDStageActor);
	TArray<FCameraInfo> GetCamerasFromUSDStage();
	
	void TraverseAndCollectCameras(UE::FUsdPrim& CurrentPrim, TArray<UE::FSdfPath>& OutCameraPaths);
	// void FUSDCameraFrameRangesModule::TraverseAndCollectCameras(const UE::FUsdPrim& CurrentPrim,
	// TArray<UE::FSdfPath>& OutCameraPaths, TArray<AActor*>& CineCameraActors, TArray<ACineCameraActor*>& OutCameraActors);
	
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
