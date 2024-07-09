// Fill out your copyright notice in the Description page of Project Settings.


#include "USDAttributeFunctionLibrary.h"

#include "USDIncludesStart.h"

#include "UsdWrappers/UsdStage.h"
#include "USDStageActor.h"
#include "UsdWrappers/UsdAttribute.h"
#include "UsdWrappers/UsdPrim.h"
#include "pxr/pxr.h"
#include "pxr/base/vt/value.h"
#include "USDIncludesEnd.h"


template <typename T>
T UUSDAttributeFunctionLibrary::GetUsdAttributeInternal(AUsdStageActor* StageActor, FString PrimName, FString AttrName)
{
	UE::FUsdStage StageBase = StageActor->GetUsdStage();

	UE::FSdfPath PrimPath;
	UE::FUsdPrim root = StageBase.GetPseudoRoot();

	GetSdfPathWithName(root, PrimName, PrimPath);

	UE::FUsdPrim CurrentPrim = StageBase.GetPrimAtPath(PrimPath);
	const TCHAR* AttrNameTChar = *AttrName;

	UE::FUsdAttribute Attr = CurrentPrim.GetAttribute(AttrNameTChar);

	UE::FVtValue Value;

	bool bSuccess = Attr.Get(Value);

	if (bSuccess)
	{
		pxr::VtValue& PxrValue = Value.GetUsdValue();
		if (PxrValue.IsHolding<T>())
		{
			return PxrValue.Get<T>();
		}
	}

	return T();
}

float UUSDAttributeFunctionLibrary::GetUsdFloatAttribute(AUsdStageActor* StageActor, FString PrimName, FString AttrName)
{
	return GetUsdAttributeInternal<float>(StageActor, PrimName, AttrName);
}

int UUSDAttributeFunctionLibrary::GetUsdIntAttribute(AUsdStageActor* StageActor, FString PrimName, FString AttrName)
{
	return GetUsdAttributeInternal<int>(StageActor, PrimName, AttrName);
}

// double UUSDAttributeFunctionLibrary::GetUsdDoubleAttribute(AUsdStageActor* StageActor, FString PrimName, FString AttrName)
// {
// 	return GetUsdAttributeInternal<double>(StageActor, PrimName, AttrName);
// }


void UUSDAttributeFunctionLibrary::GetSdfPathWithName(UE::FUsdPrim& CurrentPrim, FString TargetName, UE::FSdfPath& OutPath)
{
	if (CurrentPrim.GetName().ToString().Equals(TargetName))
	{
		OutPath = CurrentPrim.GetPrimPath();
	}
	
	
	for (UE::FUsdPrim& Child : CurrentPrim.GetChildren())
	{
		GetSdfPathWithName(CurrentPrim, TargetName, OutPath);
	}
}

// explicit instantiations to ensure proper linkage
template float UUSDAttributeFunctionLibrary::GetUsdAttributeInternal<float>(AUsdStageActor* StageActor, FString PrimName, FString AttrName);
template int UUSDAttributeFunctionLibrary::GetUsdAttributeInternal<int>(AUsdStageActor* StageActor, FString PrimName, FString AttrName);
template double UUSDAttributeFunctionLibrary::GetUsdAttributeInternal<double>(AUsdStageActor* StageActor, FString PrimName, FString AttrName);