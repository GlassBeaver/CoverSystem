// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#include "CoverSystem/ChangeNotifyingRecastNavMesh.h"
#include "NavigationSystem.h"
#include "Engine/World.h"

DECLARE_LOG_CATEGORY_EXTERN(OurNavMesh, Log, All);
DEFINE_LOG_CATEGORY(OurNavMesh)

AChangeNotifyingRecastNavMesh::AChangeNotifyingRecastNavMesh() : Super()
{}

AChangeNotifyingRecastNavMesh::AChangeNotifyingRecastNavMesh(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

void AChangeNotifyingRecastNavMesh::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->GetTimerManager().SetTimer(TileUpdateTimerHandle, this, &AChangeNotifyingRecastNavMesh::ProcessQueuedTiles, TileBufferInterval, true);
	//TODO: remove? started getting double registrations after the mission selector integration
	UNavigationSystemV1::GetCurrent(GetWorld())->OnNavigationGenerationFinishedDelegate.RemoveDynamic(this, &AChangeNotifyingRecastNavMesh::OnNavmeshGenerationFinishedHandler);
	UNavigationSystemV1::GetCurrent(GetWorld())->OnNavigationGenerationFinishedDelegate.AddDynamic(this, &AChangeNotifyingRecastNavMesh::OnNavmeshGenerationFinishedHandler); // avoid a name clash with OnNavMeshGenerationFinished()
}

void AChangeNotifyingRecastNavMesh::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(TileUpdateTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AChangeNotifyingRecastNavMesh::OnNavMeshTilesUpdated(const TArray<uint32>& ChangedTiles)
{
	Super::OnNavMeshTilesUpdated(ChangedTiles);

	TSet<uint32> updatedTiles;
	for (uint32 changedTile : ChangedTiles)
	{
		updatedTiles.Add(changedTile);
		UpdatedTilesIntervalBuffer.Add(changedTile);
		UpdatedTilesUntilFinishedBuffer.Add(changedTile);
	}

	// fire the immediate delegate
	NavmeshTilesUpdatedImmediateDelegate.Broadcast(updatedTiles);
}

void AChangeNotifyingRecastNavMesh::ProcessQueuedTiles()
{
	FScopeLock TileUpdateLock(&TileUpdateLockObject);
	if (UpdatedTilesIntervalBuffer.Num() > 0)
	{
		UE_LOG(OurNavMesh, Log, TEXT("OnNavMeshTilesUpdated - tile count: %d"), UpdatedTilesIntervalBuffer.Num());
		NavmeshTilesUpdatedBufferedDelegate.Broadcast(UpdatedTilesIntervalBuffer);
		UpdatedTilesIntervalBuffer.Empty();
	}
}

void AChangeNotifyingRecastNavMesh::OnNavmeshGenerationFinishedHandler(ANavigationData* NavData)
{
	FScopeLock TileUpdateLock(&TileUpdateLockObject);
	NavmeshTilesUpdatedUntilFinishedDelegate.Broadcast(UpdatedTilesUntilFinishedBuffer);
	UpdatedTilesUntilFinishedBuffer.Empty();
}
