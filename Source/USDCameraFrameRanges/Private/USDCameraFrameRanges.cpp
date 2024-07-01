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
#include "pxr/pxr.h"
#include "pxr/usd/usd/attribute.h"
// #include "pxr/base/gf/vec3d.h" 
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

FReply FUSDCameraFrameRangesModule::OnDuplicateButtonClicked(TObjectPtr<AUsdStageActor> StageActor, FCameraInfo Camera)
{
	UE_LOG(LogTemp, Log, TEXT("Duplicate button clicked for camera: %s"), *Camera.CameraName);
	
	TObjectPtr<ACineCameraActor> NewCameraActor = GEditor->GetEditorWorldContext().World()->SpawnActor<ACineCameraActor>();
	if (!NewCameraActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn new CineCameraActor"));
		return FReply::Handled();
	}
	FString NewLabel = Camera.CameraName + TEXT("_duplicate");
	NewCameraActor->SetActorLabel(NewLabel);
	
	UE::FUsdPrim CurrentPrim = StageActor->GetUsdStage().GetPrimAtPath(*Camera.PrimPath);
	
	UE::FVtValue Value;
	bool bSuccess = Camera.Translation.Get(Value, 0.0);
	if (bSuccess)
	{
		pxr::VtValue& PxrValue = Value.GetUsdValue();
		if (PxrValue.IsHolding<pxr::GfVec3d>())
		{
			auto Translation = PxrValue.Get<pxr::GfVec3d>();
		
			// Convert pxr::GfVec3d to FVector
			FVector CameraLocation(Translation[0], Translation[1], Translation[2]);
		
			// Set the camera location in Unreal Engine
			NewCameraActor->SetActorLocation(CameraLocation);
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

	// C:\Program Files\Epic Games\UE_5.4\Engine\Plugins\Importers\USDImporter\Source\ThirdParty\USD\include\pxr\base\gf\vec3d.h
	

	
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
		
		if (CameraInfo.RotTimeSamples.Num() > 0 && CameraInfo.TransTimeSamples.Num() > 0)
		{
			if (CameraInfo.RotTimeSamples.Num() > CameraInfo.TransTimeSamples.Num())
			{
				CameraInfo.StartFrame = CameraInfo.RotTimeSamples[0];
				CameraInfo.EndFrame = CameraInfo.RotTimeSamples[CameraInfo.RotTimeSamples.Num()-1];
			}
			else
			{
				CameraInfo.StartFrame = CameraInfo.TransTimeSamples[0];
				CameraInfo.EndFrame = CameraInfo.TransTimeSamples[CameraInfo.TransTimeSamples.Num()-1];
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Camera %s has no rotation or translation time samples"), *CameraInfo.CameraName);
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