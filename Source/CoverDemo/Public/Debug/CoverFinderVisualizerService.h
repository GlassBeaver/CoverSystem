// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BlackBoardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"
#include "CoverFinderVisData.h"
#include "CoverFinderVisualizerService.generated.h"

/**
 * Visualizes UCoverFinderService via the UCoverFinderVisData kept in the BB.
 */
UCLASS()
class COVERDEMO_API UCoverFinderVisualizerService : public UBTService
{
	GENERATED_BODY()

private:
	const FName Key_VisData = FName("VisData");
	const FName Key_GenericDebug = FName("DrawDebug");
	const FName Key_UnitDebug = FName("bDebug");
	const FName Key_DebugVisDuration = FName("DebugVisDuration");

public:
	void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds);
};
