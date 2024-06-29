// Copyright Epic Games, Inc. All Rights Reserved.

#include "USDCameraFrameRanges.h"

#include "AITestsCommon.h"
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
#include "USDIncludesStart.h"
#include "USDIncludesEnd.h"


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
					.Text(FText::FromString("Camera.FrameRange")) 
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Preview")))
					// Add functionality here when needed
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Duplicate")))
					// Add functionality here when needed
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
	AUsdStageActor* StageActor = Cast<AUsdStageActor>(StageActors[0]);

	// AUsdStageActor* StageActor = FindActor<AUsdStageActor>(World);



	if (!StageActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("USDStageActor pointer is null"));
		return Cameras;
	}

	UE::FUsdStage StageBase = StageActor->GetUsdStage();

	UE::FUsdPrim root = StageBase.GetRootLayer();
	StageBase.GetPseudoRoot()

	// Get all child actors of the USDStageActor
	// TArray<AActor*> ChildActors;
	// StageActor->GetAttachedActors(ChildActors);
	//
	// for (AActor* Actor : ChildActors)
	// {
	// 	if (ACineCameraActor* CameraActor = Cast<ACineCameraActor>(Actor))
	// 	{
	// 		FCameraInfo CameraInfo;
	// 		CameraInfo.CameraName = CameraActor->GetName();
	// 		CameraInfo.StartFrame = 0;
	// 		CameraInfo.EndFrame=100;
	// 		Cameras.Add(CameraInfo);
	// 	}
	// }
	
	return Cameras;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUSDCameraFrameRangesModule, USDCameraFrameRanges)