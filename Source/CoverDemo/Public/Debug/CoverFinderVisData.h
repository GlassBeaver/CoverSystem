// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DebugPoint.h"
#include "DebugArrow.h"
#include "CoverFinderVisData.generated.h"

/**
 * Visualization data for debugging the cover finder.
 */
UCLASS()
class COVERDEMO_API UCoverFinderVisData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FDebugPoint> DebugPoints;

	UPROPERTY()
	TArray<FDebugArrow> DebugArrows;
};
