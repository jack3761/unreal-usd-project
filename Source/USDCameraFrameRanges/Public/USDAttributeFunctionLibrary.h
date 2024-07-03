// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "USDAttributeFunctionLibrary.generated.h"

namespace UE
{
	class FUsdPrim;
	class FSdfPath;
}

class AUsdStageActor;

/**
 * Library of functions to access USD Attributes of different types
 */
UCLASS()
class USDCAMERAFRAMERANGES_API UUSDAttributeFunctionLibrary : public UBlueprintFunctionLibrary
{

public:
	UFUNCTION(BluePrintCallable, Category = "USD")
	static float GetUsdFloatAttribute(AUsdStageActor* StageActor, FString PrimName, FString AttrName);

private:
	static void GetSdfPathWithName(UE::FUsdPrim& CurrentPrim, FString TargetName, UE::FSdfPath& OutPath);


	
	GENERATED_BODY()
};
