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


float UUSDAttributeFunctionLibrary::GetUsdFloatAttribute(AUsdStageActor* StageActor, FString PrimName, FString AttrName)
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
		if (PxrValue.IsHolding<float>())
		{
			float AttrValue = PxrValue.Get<float>();
			return AttrValue;
		}
	}

	return 0.0;
}

FString UUSDAttributeFunctionLibrary::GetPointlessMessage()
{
	return FString(TEXT("This is a pointless message"));
}

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

