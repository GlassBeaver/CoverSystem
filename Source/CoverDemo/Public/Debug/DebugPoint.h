// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DebugPoint.generated.h"

USTRUCT()
struct FDebugPoint
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FColor Color;

	UPROPERTY()
	bool bGenericOrUnitDebugData;

	FDebugPoint()
		: Location(), Color(), bGenericOrUnitDebugData()
	{}

	FDebugPoint(FVector _Location, FColor _Color, bool _bGenericOrUnitDebugData)
		: Location(_Location), Color(_Color), bGenericOrUnitDebugData(_bGenericOrUnitDebugData)
	{}
};
