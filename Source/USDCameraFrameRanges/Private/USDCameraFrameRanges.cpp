// Copyright Epic Games, Inc. All Rights Reserved.

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

TSharedRef<SDockTab> FUSDCameraFrameRangesModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	// Dummy data for demonstration
	TArray<FCameraInfo> Cameras = GetCamerasFromUSDStage();
	
	
	// Cameras.Add({ "Camera1", 1, 100});
	// Cameras.Add({ "Camera2", 50, 200});
	// // Add more cameras as needed

	// Create a vertical box to hold all camera rows
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
					// Add functionality 
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

TArray<FCameraInfo> FUSDCameraFrameRangesModule::GetCamerasFromUSDStage()
{
	TArray<UE::FSdfPath> CameraPaths;
	TArray<FCameraInfo> Cameras;
	
	if (!GEditor)
	{
		UE_LOG(LogTemp, Warning, TEXT("GEditor is not available"));
		return Cameras;
	}
	
	UWorld* World = GEditor->GetEditorWorldContext().World();


	// Find all USDStageActor actors in the world
	TArray<AActor*> StageActors;
	UGameplayStatics::GetAllActorsOfClass(World, AUsdStageActor::StaticClass(), StageActors);

	// will only work with one UsdStageActor, which is the aim for this project
	AUsdStageActor* StageActor = Cast<AUsdStageActor>(StageActors[0]);

	

	// AUsdStageActor* StageActor = FindActor<AUsdStageActor>(World);

	if (!StageActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("USDStageActor pointer is null"));
		return Cameras;
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("USDStageActor assigned"));
	}

	UE::FUsdStage StageBase = StageActor->GetUsdStage();
	
	UE::FUsdPrim root = StageBase.GetPseudoRoot();

	TArray<AActor*> CineCameraActors;
	UGameplayStatics::GetAllActorsOfClass(World, ACineCameraActor::StaticClass(), CineCameraActors);

	TArray<ACineCameraActor*> OutCameraActors;

	// TraverseAndCollectCameras(root, CameraPaths, CineCameraActors, OutCameraActors);
	TraverseAndCollectCameras(root, CameraPaths);

	for (UE::FSdfPath path : CameraPaths)
	{
		UE::FUsdPrim CurrentPrim = StageBase.GetPrimAtPath(path);
		FCameraInfo CameraInfo;
		CameraInfo.CameraName = CurrentPrim.GetName().ToString();
		
		UE::FUsdAttribute AttrRotate = CurrentPrim.GetAttribute(TEXT("xformOp:rotateXYZ"));
		UE::FUsdAttribute AttrTranslate = CurrentPrim.GetAttribute(TEXT("xformOp:translate"));

		TArray<double> RotTimeSamples;
		AttrRotate.GetTimeSamples(RotTimeSamples);

		TArray<double> TransTimeSamples;
		AttrRotate.GetTimeSamples(TransTimeSamples);

		if (RotTimeSamples.Num() > TransTimeSamples.Num())
		{
			CameraInfo.StartFrame = RotTimeSamples[0];
			CameraInfo.EndFrame = RotTimeSamples[RotTimeSamples.Num()-1];
		}
		else
		{
			CameraInfo.StartFrame = TransTimeSamples[0];
			CameraInfo.EndFrame = TransTimeSamples[TransTimeSamples.Num()-1];
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
// void FUSDCameraFrameRangesModule::TraverseAndCollectCameras(const UE::FUsdPrim& CurrentPrim,
// 	TArray<UE::FSdfPath>& OutCameraPaths, TArray<AActor*>& CineCameraActors, TArray<ACineCameraActor*>& OutCameraActors)
// {
//
// 	if (CurrentPrim.IsA(FName(TEXT("Camera"))))
// 	{
// 		FString CameraName = CurrentPrim.GetName().ToString();
// 		for (TObjectPtr<AActor> Camera : CineCameraActors)
// 		{
// 			if (Camera->GetActorLabel().Equals(CameraName))
// 			{
// 				OutCameraActors.Add(Cast<ACineCameraActor>(Camera));
// 				
// 				UE::FSdfPath path = CurrentPrim.GetPrimPath();				
// 				OutCameraPaths.Add(path);
// 		        UE_LOG(LogTemp, Log, TEXT("Camera found at path: %s"), *path.GetString());
// 			}
// 		}
// 	}
//
// 	for (UE::FUsdPrim& Child : CurrentPrim.GetChildren())
// 	{
// 		UE_LOG(LogTemp, Log, TEXT("Traversing from %s, type: %s"), *Child.GetName().ToString(), *Child.GetTypeName().ToString());
// 		TraverseAndCollectCameras(Child, OutCameraPaths, CineCameraActors, OutCameraActors);
// 	}
// }

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUSDCameraFrameRangesModule, USDCameraFrameRanges)