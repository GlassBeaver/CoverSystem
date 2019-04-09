// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DebugArrow.generated.h"

USTRUCT()
struct FDebugArrow
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	FVector Start;

	UPROPERTY()
	FVector End;

	UPROPERTY()
	FColor Color;

	UPROPERTY()
	bool bGenericOrUnitDebugData;

	FDebugArrow()
		: Start(), End(), Color(), bGenericOrUnitDebugData()
	{}

	FDebugArrow(FVector _Start, FVector _End, FColor _Color, bool _bGenericOrUnitDebugData)
		: Start(_Start), End(_End), Color(_Color), bGenericOrUnitDebugData(_bGenericOrUnitDebugData)
	{}
};
