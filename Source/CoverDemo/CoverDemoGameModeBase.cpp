// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#include "CoverDemoGameModeBase.h"
#include "CoverSystem/CoverSystem.h"
#include "CoverSystem/CoverOctree.h"
#include "CoverSystem/CoverPointOctreeElement.h"
#include "CoverSystem/ChangeNotifyingRecastNavMesh.h"

#if DEBUG_RENDERING
#include "DrawDebugHelpers.h"
#endif

ACoverDemoGameModeBase::ACoverDemoGameModeBase() : Super()
{
	//FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &ACoverDemoGameModeBase::OnWorldInitDelegate);
}

void ACoverDemoGameModeBase::PostLoad()
{
	Super::PostLoad();
}

void ACoverDemoGameModeBase::PostActorCreated()
{
	Super::PostActorCreated();

	// DEPRECATED: doesn't seem to work or matter anymore in 4.24
	// register our custom navmesh subclass - this is the only thing to do to get it picked up by UNavigationSystem::RegisterNavigationDataInstances()
	// that and creating a custom nav agent under project settings -> navigation system -> agents -> supported agents, whose navigation data class and preferred nav data should be set to our class.
	//AChangeNotifyingRecastNavMesh* navmesh = GetWorld()->SpawnActor<AChangeNotifyingRecastNavMesh>(AChangeNotifyingRecastNavMesh::StaticClass());
	//check(IsValid(navmesh));
}

void ACoverDemoGameModeBase::OnWorldInitDelegate(UWorld* World, const UWorld::InitializationValues IVS)
{
}

void ACoverDemoGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}

void ACoverDemoGameModeBase::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ACoverDemoGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	CoverSystem->bShutdown = false;
	CoverSystem = UCoverSystem::GetInstance(GetWorld()); // see the CoverSystem variable for why this is here
	CoverSystem->MapBounds = MapBounds;
#if DEBUG_RENDERING
	CoverSystem->bDebugDraw = bDebugDraw;
#endif
}

void ACoverDemoGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	CoverSystem->bShutdown = true;
	CoverSystem = nullptr;
}


void ACoverDemoGameModeBase::DebugShowCoverPoints()
{
#if DEBUG_RENDERING
	UWorld* world = GetWorld();
	FlushPersistentDebugLines(world);
	TArray<FCoverPointOctreeElement> coverPoints;

	if (UCoverSystem::bShutdown)
		return;
	UCoverSystem::GetInstance(world)->FindCoverPoints(coverPoints, FBoxCenterAndExtent(FVector::ZeroVector, FVector(64000.0f)).GetBox());

	for (FCoverPointOctreeElement cp : coverPoints)
		DrawDebugSphere(world, cp.Data->Location, 25, 4, cp.Data->bTaken ? FColor::Red : cp.Data->bForceField ? FColor::Orange : FColor::Blue, true, -1, 0, 5);
#endif
}

void ACoverDemoGameModeBase::ForceGC()
{
#if DEBUG_RENDERING
	GEngine->ForceGarbageCollection(true);
#endif
}
