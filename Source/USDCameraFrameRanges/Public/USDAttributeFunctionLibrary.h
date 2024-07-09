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
	GENERATED_BODY()
	
public:
	// Templated function for internal use
	template <typename T>
	static T GetUsdAttributeInternal(AUsdStageActor* StageActor, FString PrimName, FString AttrName);

	// BlueprintCallable functions
	UFUNCTION(BlueprintCallable, Category = "USD Attributes")
	static float GetUsdFloatAttribute(AUsdStageActor* StageActor, FString PrimName, FString AttrName);

	UFUNCTION(BlueprintCallable, Category = "USD Attributes")
	static int GetUsdIntAttribute(AUsdStageActor* StageActor, FString PrimName, FString AttrName);

	// UFUNCTION(BlueprintCallable, Category = "USD Attributes")
	// static double GetUsdDoubleAttribute(AUsdStageActor* StageActor, FString PrimName, FString AttrName);


private:
	static void GetSdfPathWithName(UE::FUsdPrim& CurrentPrim, FString TargetName, UE::FSdfPath& OutPath);

	
};



