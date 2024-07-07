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
#include "UsdWrappers/UsdRelationship.h"
#include "pxr/pxr.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/base/vt/value.h"

#include "pxr/usd/usdGeom/xform.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/shader.h"
#include "USDIncludesEnd.h"
#include "Engine/ObjectLibrary.h"
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

    if (!StageActor)
    {
        // Handle case when StageActor is not found
        return SNew(SDockTab)
            .TabRole(ETabRole::NomadTab)
            [
                SNew(SBox)
                .Padding(20)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("USD Stage Actor not found. Please ensure a USD Stage Actor is present in the scene.")))
                ]
            ];
    }

    TArray<FCameraInfo> Cameras = GetCamerasFromUSDStage(StageActor);

    if (Cameras.Num() == 0)
    {
        // Handle case when no cameras are found
        return SNew(SDockTab)
            .TabRole(ETabRole::NomadTab)
            [
                SNew(SBox)
                .Padding(20)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("No cameras found in the USD Stage. Please ensure there are cameras in the USD Stage.")))
                ]
            ];
    }

    TSharedPtr<SVerticalBox> CameraList = SNew(SVerticalBox);

    CameraList->AddSlot()
    .Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(0.4)
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
                // Add functionality for the Preview button
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

	return SNew(SDockTab)
	.TabRole(ETabRole::NomadTab)
	[
		// Main content of the tab
		SNew(SVerticalBox) // Use SVerticalBox to hold everything vertically
		+ SVerticalBox::Slot()
		.AutoHeight()
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
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(20)
		[
			// Temporary button to test functionality
			SNew(SButton)
			.Text(FText::FromString(TEXT("Material swap")))
			.OnClicked(FOnClicked::CreateRaw(this, &FUSDCameraFrameRangesModule::OnMaterialSwapButtonClicked, StageActor))
			
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

FReply FUSDCameraFrameRangesModule::OnMaterialSwapButtonClicked(TObjectPtr<AUsdStageActor> StageActor)
{
	UE::FUsdStage Stage = StageActor->GetUsdStage();
	UE::FUsdPrim root = Stage.GetPseudoRoot();
	TArray<FMaterialInfo> MaterialNames;

	// TArray<UMaterialInstance*>* Materials = GetMaterialInstances();
	TArray<UMaterial*>* FoundMaterials = GetAllMaterials();
	
	TraverseAndCollectMaterials(StageActor, root, MaterialNames);

	if (MaterialNames.Num() >0 )
	{
		for (FMaterialInfo Mat : MaterialNames)
		{
			UE::FUsdPrim CurrentPrim = Stage.GetPrimAtPath(Mat.PrimPath);
			USceneComponent* GeneratedComponent = StageActor->GetGeneratedComponent(Mat.PrimPath.GetString());
			
			if (GeneratedComponent)
			{
				UE_LOG(LogTemp, Log, TEXT("ObjName: %s Generated Component name: %s"), *Mat.ObjName, *GeneratedComponent->GetName());

				// Find the Unreal Material with the same name as the USD material
				FString MaterialName = Mat.MatName;
				for (UMaterial* FoundMaterial : *FoundMaterials)
				{
					if (MaterialName.Equals(FoundMaterial->GetName()))
					{
						UMeshComponent* MeshComponent = Cast<UMeshComponent>(GeneratedComponent);

						MeshComponent->SetMaterial(0, FoundMaterial);
						UE_LOG(LogTemp, Log, TEXT("Assigned material: %s to component: %s"), *MaterialName, *MeshComponent->GetName());
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("Material: %s not found in project"), *MaterialName);
					}
				}
			}
		}
	}
	return FReply::Handled();
}

// https://forums.unrealengine.com/t/plugin-get-all-materials-in-current-project/342793/10
TArray<UMaterialInstance*>* FUSDCameraFrameRangesModule::GetMaterialInstances()
{
	TArray<UMaterialInstance*>* Materials = new TArray<UMaterialInstance*>();

    UObjectLibrary *lib = UObjectLibrary::CreateLibrary(UObject::StaticClass(), false, true);
	UE_LOG(LogTemp, Warning, TEXT("Searching for material instances in /Game/Materials..."));
	lib->LoadAssetDataFromPath(TEXT("/Game/Materials"));
	TArray<FAssetData> assetData;
	lib->GetAssetDataList(assetData);
	UE_LOG(LogTemp, Warning, TEXT("Found %d"), assetData.Num());

	for (FAssetData asset : assetData) {
		UMaterialInstance* mi = Cast<UMaterialInstance>(asset.GetAsset());
		if (mi) {
			UE_LOG(LogTemp, Warning, TEXT("Material instance %s"), *mi->GetName());
			Materials->Add(mi);
		}
	}
	return Materials;
}

TArray<UMaterial*>* FUSDCameraFrameRangesModule::GetAllMaterials()
{
	TArray<UMaterial*>* Assets = new TArray<UMaterial*>();

	// Create a library to load all asset data, not filtered by a specific class
	UObjectLibrary *lib = UObjectLibrary::CreateLibrary(UObject::StaticClass(), false, true);
	UE_LOG(LogTemp, Warning, TEXT("Searching for assets in /Game/Materials..."));

	// Load asset data from the specified path
	lib->LoadAssetDataFromPath(TEXT("/Game/Materials"));

	// Retrieve asset data list
	TArray<FAssetData> assetData;
	lib->GetAssetDataList(assetData);
	UE_LOG(LogTemp, Warning, TEXT("Found %d assets"), assetData.Num());

	// Iterate through all asset data and add to the Assets array
	for (FAssetData asset : assetData) {
		UMaterial* obj = Cast<UMaterial>(asset.GetAsset());
		if (obj) {
			UE_LOG(LogTemp, Warning, TEXT("Asset: %s, type: %s"), *obj->GetName(), *obj->GetClass()->GetName());
			Assets->Add(obj);
		}
	}

	return Assets;
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

	TArray<AActor*> StageActors;
	UGameplayStatics::GetAllActorsOfClass(World, AUsdStageActor::StaticClass(), StageActors);

	if (StageActors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No AUsdStageActor found in the world"));
		return nullptr;
	}
	else if (StageActors.Num() > 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Multiple UsdStageActor's detected, using first one found"));
		return nullptr;
	}
		

	// Will only work with one UsdStageActor, which is the aim for this project
	TObjectPtr<AUsdStageActor> StageActor = Cast<AUsdStageActor>(StageActors[0]);
	if (!StageActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("USDStageActor pointer is null"));
		return nullptr;
	}
    
	UE_LOG(LogTemp, Log, TEXT("USDStageActor assigned"));
	return StageActor;
}


// TODO add protection against array length stuff
TArray<FCameraInfo> FUSDCameraFrameRangesModule::GetCamerasFromUSDStage(TObjectPtr<AUsdStageActor> StageActor)
{
    TArray<FCameraInfo> Cameras;

    if (!StageActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("StageActor is null."));
        return Cameras;
    }

    UE::FUsdStage StageBase = StageActor->GetUsdStage();
    UE::FUsdPrim root = StageBase.GetPseudoRoot();

    TArray<UE::FSdfPath> CameraPaths;
    TraverseAndCollectCameras(root, CameraPaths);

    if (CameraPaths.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No cameras found in the USD Stage."));
        return Cameras;
    }

    for (UE::FSdfPath path : CameraPaths)
    {
        UE::FUsdPrim CurrentPrim = StageBase.GetPrimAtPath(path);
        if (!CurrentPrim)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to get Prim at path: %s"), *path.GetString());
            continue;
        }

        FCameraInfo CameraInfo;
        CameraInfo.CameraName = CurrentPrim.GetName().ToString();
        
        CameraInfo.Translation = CurrentPrim.GetAttribute(TEXT("xformOp:translate"));
        CameraInfo.Rotation = CurrentPrim.GetAttribute(TEXT("xformOp:rotateXYZ"));

        if (CameraInfo.Rotation && CameraInfo.Translation)
        {
            CameraInfo.Rotation.GetTimeSamples(CameraInfo.RotTimeSamples);
            CameraInfo.Translation.GetTimeSamples(CameraInfo.TransTimeSamples);
            
            if (CameraInfo.RotTimeSamples.Num() > 1 || CameraInfo.TransTimeSamples.Num() > 1)
            {
                if (CameraInfo.RotTimeSamples.Num() > CameraInfo.TransTimeSamples.Num())
                {
                    if (CameraInfo.RotTimeSamples[1] == 2.0)
                        CameraInfo.StartFrame = CameraInfo.RotTimeSamples[0];
                    else
                        CameraInfo.StartFrame = CameraInfo.RotTimeSamples[1];

                    CameraInfo.EndFrame = CameraInfo.RotTimeSamples.Last();
                }
                else
                {
                    if (CameraInfo.RotTimeSamples[1] == 2.0)
                        CameraInfo.StartFrame = CameraInfo.RotTimeSamples[0];
                    else
                        CameraInfo.StartFrame = CameraInfo.RotTimeSamples[1];

                    CameraInfo.EndFrame = CameraInfo.TransTimeSamples.Last();
                }
            }
            else
            {
                CameraInfo.StartFrame = 1;
                CameraInfo.EndFrame = 1;
            }

            Cameras.Add(CameraInfo);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to get necessary attributes for camera: %s"), *CameraInfo.CameraName);
        }
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


void FUSDCameraFrameRangesModule::TraverseAndCollectMaterials(TObjectPtr<AUsdStageActor> StageActor, UE::FUsdPrim& CurrentPrim, TArray<FMaterialInfo>& MaterialNames)
{
	UE::FUsdStage Stage = StageActor->GetUsdStage();
	if (!CurrentPrim)
	{
		return;
	}

	
	const TCHAR* RelationshipName = TEXT("material:binding");
	if (UE::FUsdRelationship MaterialBindingRel = CurrentPrim.GetRelationship(RelationshipName))
	{
		TArray<UE::FSdfPath> TargetPaths;
		bool bTargets = MaterialBindingRel.GetTargets(TargetPaths);
		if (bTargets)
		{
			for (const UE::FSdfPath& Path : TargetPaths)
			{
				UE_LOG(LogTemp, Log, TEXT("Current prim: %s Target path: %s"), *CurrentPrim.GetName().ToString(), *Path.GetString());

				if (UE::FUsdPrim MaterialPrim = Stage.GetPrimAtPath(Path))
				{
					FString MaterialName = MaterialPrim.GetName().ToString();
					UE_LOG(LogTemp, Log, TEXT("Material found: %s"), *MaterialName);

					for (UE::FUsdPrim ChildPrim : MaterialPrim.GetChildren())
					{
						if (ChildPrim.IsA("Shader"))
						{
							FString ShaderName = ChildPrim.GetName().ToString();
							UE_LOG(LogTemp, Log, TEXT("Shader found: %s"), *ShaderName);

							FMaterialInfo MaterialInfo;
							MaterialInfo.ObjName = CurrentPrim.GetName().ToString();
							MaterialInfo.MatName = ShaderName;
							MaterialInfo.PrimPath = CurrentPrim.GetPrimPath();

							UE_LOG(LogTemp, Log, TEXT("Adding material info, ObjName: %s MatName: %s PrimPath: %s"), *MaterialInfo.ObjName,  *MaterialInfo.MatName, *MaterialInfo.PrimPath.GetString());

							
							MaterialNames.Add(MaterialInfo);
						}
					}
				}
			}
		}
	}
	// if (pxr::UsdShadeMaterial Material = pxr::UsdShadeMaterial::Get(Stage, CurrentPrim.GetPrimPath()))
	// {
	// 	// Collect the material information
	// 	FMaterialInfo MaterialInfo;
	// 	pxr::UsdPrim MatPrim = Material.GetPrim();
	// 	MaterialInfo.ObjName = CurrentPrim.GetName().ToString();
	// 	MaterialInfo.MatName = FString(MatPrim.GetPrimPath().GetName().c_str());
	// 	MaterialInfo.PrimPath = CurrentPrim.GetPrimPath();
	//
	// 	UE_LOG(LogTemp, Log, TEXT("Material found, object name: %s material name: %s prim path: %s"), *MaterialInfo.ObjName, *MaterialInfo.MatName, *MaterialInfo.PrimPath.GetString() );
	//
	// 	MaterialNames.Add(MaterialInfo);
	//
	// }

	for (pxr::UsdPrim ChildPrim : CurrentPrim.GetChildren())
	{
		UE::FUsdPrim UEChildPrim(ChildPrim);
		TraverseAndCollectMaterials(StageActor, UEChildPrim, MaterialNames);
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUSDCameraFrameRangesModule, USDCameraFrameRanges)