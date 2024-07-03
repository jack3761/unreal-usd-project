﻿// Copyright Epic Games, Inc. All Rights Reserved.

#include "USDCameraFrameRanges.h"

#include "USDCameraFrameRangesStyle.h"
#include "USDCameraFrameRangesCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "USDStageActor.h"
#include "CineCameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "Editor.h"
#include "EditorLevelUtils.h"

#include "USDIncludesStart.h"
#include "UsdWrappers/UsdStage.h"
#include "UsdWrappers/UsdPrim.h"
#include "UsdWrappers/UsdAttribute.h"
#include "pxr/pxr.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/base/vt/value.h"

#include "pxr/usd/usdGeom/xform.h"
#include "pxr/base/gf/vec3d.h"
#include "USDIncludesEnd.h"
#include "Experimental/Async/AwaitableTask.h"



static const FName USDCameraFrameRangesTabName("USDCameraFrameRanges");

#define LOCTEXT_NAMESPACE "FUSDCameraFrameRangesModule"

void FUSDCameraFrameRangesModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FUSDCameraFrameRangesStyle::Initialize();
	FUSDCameraFrameRangesStyle::ReloadTextures();

	FUSDCameraFrameRangesCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FUSDCameraFrameRangesCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FUSDCameraFrameRangesModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FUSDCameraFrameRangesModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(USDCameraFrameRangesTabName, FOnSpawnTab::CreateRaw(this, &FUSDCameraFrameRangesModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FUSDCameraFrameRangesTabTitle", "USDCameraFrameRanges"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FUSDCameraFrameRangesModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FUSDCameraFrameRangesStyle::Shutdown();

	FUSDCameraFrameRangesCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(USDCameraFrameRangesTabName);
}

// TODO add protection if there isn't USD or cameras
TSharedRef<SDockTab> FUSDCameraFrameRangesModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{

	TObjectPtr<AUsdStageActor> StageActor = GetUsdStageActor();
	
	TArray<FCameraInfo> Cameras = GetCamerasFromUSDStage(StageActor);	
	
	TSharedPtr<SVerticalBox> CameraList = SNew(SVerticalBox);
	
	CameraList->AddSlot()
	.Padding((2))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth((0.4))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Camera name")))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.4)
		[
			SNew(STextBlock)
			.Text(FText::FromString("Frame range"))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Create CineCameraActor")))
		]
	];
	
	if (Cameras.Num()>0)
	{
		// Loop through Cameras array and create a row widget for each camera
		for (const FCameraInfo& Camera : Cameras)
		{
			CameraList->AddSlot()
			.Padding(2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.4)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Camera.CameraName))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.4)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%d - %d"), Camera.StartFrame, Camera.EndFrame)))
	
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Preview")))
					// Add functionality
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Duplicate")))
					.OnClicked(FOnClicked::CreateRaw(this, &FUSDCameraFrameRangesModule::OnDuplicateButtonClicked, StageActor, Camera))
				]
			];
		}
	}
	
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Main content of the tab
			SNew(SBox)
			.Padding(20)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SBorder)
					.Padding(FMargin(20))
					[
						CameraList.ToSharedRef()
					]
				]
			]
		];
}

// TODO add functionality to find the aperture stuff, and add duplicate animation
FReply FUSDCameraFrameRangesModule::OnDuplicateButtonClicked(TObjectPtr<AUsdStageActor> StageActor, FCameraInfo Camera)
{
	UE_LOG(LogTemp, Log, TEXT("Duplicate button clicked for camera: %s"), *Camera.CameraName);

	UWorld* World =  GEditor->GetEditorWorldContext().World();
	
	TObjectPtr<ACineCameraActor> NewCameraActor = World->SpawnActor<ACineCameraActor>();
	
	if (!NewCameraActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn new CineCameraActor"));
		return FReply::Handled();
	}
	
	FString NewLabel = Camera.CameraName + TEXT("_duplicate");
	NewCameraActor->SetActorLabel(NewLabel);

	UE_LOG(LogTemp, Log, TEXT("New camera created with label: %s"), *NewCameraActor->GetActorLabel());
	UE_LOG(LogTemp, Log, TEXT("Camera name: %s"), *NewCameraActor->GetName());
	
	UE::FVtValue Value;
	
	UE_LOG(LogTemp, Log, TEXT("Getting value"));

	bool bSuccess = Camera.Translation.Get(Value, 0.0);

	UE_LOG(LogTemp, Log, TEXT("Value received"));

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("Translation value found"));

		pxr::VtValue& PxrValue = Value.GetUsdValue();
		if (PxrValue.IsHolding<pxr::GfVec3d>())
		{
			pxr::GfVec3d Translation = PxrValue.Get<pxr::GfVec3d>();
		
			// Convert pxr::GfVec3d to FVector
			FVector CameraLocation(Translation[0], Translation[2], Translation[1]);
		
			// Set the camera location in Unreal Engine
			NewCameraActor->SetActorLocation(CameraLocation);

			UE_LOG(LogTemp, Log, TEXT("Camera actor location set to %f %f %f"), Translation[0], Translation[1], Translation[2]);

		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("The translation attribute value is not of type GfVec3d"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to get the translation attribute at time 0"));
	}

	bSuccess = Camera.Rotation.Get(Value, 0.0);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("Rotation value found"));

		pxr::VtValue& PxrValue = Value.GetUsdValue();
		if (PxrValue.IsHolding<pxr::GfVec3f>())
		{
			pxr::GfVec3f Rotation = PxrValue.Get<pxr::GfVec3f>();
		
			// Convert pxr::GfVec3d to FVector
			FRotator CameraRotation(Rotation[0], (Rotation[1]*-1)-90, Rotation[2]);
		
			// Set the camera location in Unreal Engine
			NewCameraActor->SetActorRotation(CameraRotation);

			UE_LOG(LogTemp, Log, TEXT("Camera actor rotation set to %f %f %f"), Rotation[0], (Rotation[1]*-1)-90, Rotation[2]);

		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("The Rotation attribute value is not of type GfVec3d"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to get the Rotation attribute at time 0"));
	}

	return FReply::Handled();
}


void FUSDCameraFrameRangesModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(USDCameraFrameRangesTabName);
}

void FUSDCameraFrameRangesModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FUSDCameraFrameRangesCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FUSDCameraFrameRangesCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

TObjectPtr<AUsdStageActor> FUSDCameraFrameRangesModule::GetUsdStageActor()
{
	if (!GEditor)
	{
		UE_LOG(LogTemp, Warning, TEXT("GEditor is not available"));
		return nullptr;
	}
	
	UWorld* World = GEditor->GetEditorWorldContext().World();


	// Find all USDStageActor actors in the world
	TArray<AActor*> StageActors;
	UGameplayStatics::GetAllActorsOfClass(World, AUsdStageActor::StaticClass(), StageActors);

	// Will only work with one UsdStageActor, which is the aim for this project
	TObjectPtr<AUsdStageActor> StageActor = Cast<AUsdStageActor>(StageActors[0]);	

	if (!StageActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("USDStageActor pointer is null"));
		return nullptr;
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("USDStageActor assigned"));
	}

	return StageActor;
}

// TODO add protection against array length stuff
TArray<FCameraInfo> FUSDCameraFrameRangesModule::GetCamerasFromUSDStage(TObjectPtr<AUsdStageActor> StageActor)
{
	TArray<UE::FSdfPath> CameraPaths;
	TArray<FCameraInfo> Cameras;
	
	UE::FUsdStage StageBase = StageActor->GetUsdStage();	
	UE::FUsdPrim root = StageBase.GetPseudoRoot();

	TraverseAndCollectCameras(root, CameraPaths);

	for (UE::FSdfPath path : CameraPaths)
	{
		UE::FUsdPrim CurrentPrim = StageBase.GetPrimAtPath(path);
		FCameraInfo CameraInfo;
		CameraInfo.CameraName = CurrentPrim.GetName().ToString();
		
		// UE::FUsdAttribute AttrRotate = CurrentPrim.GetAttribute(TEXT("xformOp:rotateXYZ"));
		// UE::FUsdAttribute AttrTranslate = CurrentPrim.GetAttribute(TEXT("xformOp:translate"));
		
		CameraInfo.Translation = CurrentPrim.GetAttribute(TEXT("xformOp:translate"));
		CameraInfo.Rotation = CurrentPrim.GetAttribute(TEXT("xformOp:rotateXYZ"));

		// TArray<double> RotTimeSamples;
		// AttrRotate.GetTimeSamples(RotTimeSamples);
		//
		// TArray<double> TransTimeSamples;
		// AttrRotate.GetTimeSamples(TransTimeSamples);

		CameraInfo.Rotation.GetTimeSamples(CameraInfo.RotTimeSamples);
		CameraInfo.Translation.GetTimeSamples(CameraInfo.TransTimeSamples);
		
		if (CameraInfo.RotTimeSamples.Num() > 1 || CameraInfo.TransTimeSamples.Num() > 1)
		{
			if (CameraInfo.RotTimeSamples.Num() > CameraInfo.TransTimeSamples.Num())
			{
				if (CameraInfo.RotTimeSamples[1] == 2.0)
					CameraInfo.StartFrame = CameraInfo.RotTimeSamples[0];
				else
				{
					CameraInfo.StartFrame = CameraInfo.RotTimeSamples[1];
				}
				CameraInfo.EndFrame = CameraInfo.RotTimeSamples[CameraInfo.RotTimeSamples.Num()-1];
			}
			else
			{
				if (CameraInfo.RotTimeSamples[1] == 2.0)
					CameraInfo.StartFrame = CameraInfo.RotTimeSamples[0];
				else
				{
					CameraInfo.StartFrame = CameraInfo.RotTimeSamples[1];
				}
				CameraInfo.EndFrame = CameraInfo.TransTimeSamples[CameraInfo.TransTimeSamples.Num()-1];
			}
		}
		else
		{
			CameraInfo.StartFrame = 1;
			CameraInfo.EndFrame = 1;
		}
		
		Cameras.Add(CameraInfo);
	}
	
	return Cameras;
}

void FUSDCameraFrameRangesModule::TraverseAndCollectCameras(UE::FUsdPrim& CurrentPrim,
	TArray<UE::FSdfPath>& OutCameraPaths)
{

	if (CurrentPrim.IsA(FName(TEXT("Camera"))))
	{
		UE::FSdfPath path = CurrentPrim.GetPrimPath();
		OutCameraPaths.Add(path);
        UE_LOG(LogTemp, Log, TEXT("Camera found at path: %s"), *path.GetString());
	}

	for (UE::FUsdPrim& Child : CurrentPrim.GetChildren())
	{
		UE_LOG(LogTemp, Log, TEXT("Traversing from %s, type: %s"), *Child.GetName().ToString(), *Child.GetTypeName().ToString());
		TraverseAndCollectCameras(Child, OutCameraPaths);
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUSDCameraFrameRangesModule, USDCameraFrameRanges)