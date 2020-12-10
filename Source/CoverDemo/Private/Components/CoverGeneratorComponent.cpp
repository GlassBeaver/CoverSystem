// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#include "Components/CoverGeneratorComponent.h"
#include "Tasks/ActorCoverPointGeneratorTask.h"

// Sets default values for this component's properties
UCoverGeneratorComponent::UCoverGeneratorComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UCoverGeneratorComponent::BeginPlay()
{
	Super::BeginPlay();

	FVector origin;
	FVector extent;
	GetOwner()->GetActorBounds(false, origin, extent);
	OwnerBounds = FBoxCenterAndExtent(origin, extent).GetBox();

	// generate cover points NEAR begin play, if requested
	// have to wait for the navmesh to finish generation, first
	if (bGenerateOnBeginPlay)
		UNavigationSystemV1::GetCurrent(GetWorld())->OnNavigationGenerationFinishedDelegate.AddDynamic(this, &UCoverGeneratorComponent::OnNavmeshGenerationFinished);
}

void UCoverGeneratorComponent::OnNavmeshGenerationFinished(ANavigationData* NavData)
{
	GenerateCoverPoints();
}

void UCoverGeneratorComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	// remove owner's cover points from the map upon destroy
	if (UCoverSystem::bShutdown)
		return;
	UCoverSystem::GetInstance(GetWorld())->RemoveCoverPointsOfObject(GetOwner());

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

// Called every frame
void UCoverGeneratorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCoverGeneratorComponent::GenerateCoverPoints()
{
#if DEBUG_RENDERING
	if (UCoverSystem::bShutdown)
		return;
	bDebugDraw = UCoverSystem::GetInstance(GetWorld())->bDebugDraw;
#endif

	// spawn the cover generator task
#if DEBUG_RENDERING
	if (bDebugDraw)
		(new FAutoDeleteAsyncTask<FActorCoverPointGeneratorTask>(
			GetOwner(),
			GetWorld(),
			BoundingBoxExpansion,
			ScanGridUnit,
			SmallestAgentHeight,
			bGeneratePerStaticMesh
			))->StartSynchronousTask();
	else
#endif
		(new FAutoDeleteAsyncTask<FActorCoverPointGeneratorTask>(
			GetOwner(),
			GetWorld(),
			BoundingBoxExpansion,
			ScanGridUnit,
			SmallestAgentHeight,
			bGeneratePerStaticMesh
		))->StartBackgroundTask();
}
