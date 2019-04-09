// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NavMesh/RecastNavMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ChangeNotifyingRecastNavMesh.generated.h"

// DELEGATES //

// Fired every X seconds.
// ChangedTiles contains tiles that have been updated since the last timer.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNavmeshTilesUpdatedBufferedDelegate, const TSet<uint32>&, ChangedTiles);

// Fired as tiles are updated.
// ChangedTiles contains the same tiles as what get passed around inside Recast.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNavmeshTilesUpdatedImmediateDelegate, const TSet<uint32>&, ChangedTiles);

// Fires once navigation generation is finished, i.e. there are no dirty tiles left.
// ChangedTiles contains all the tiles that have been updated since the last time nav was finished.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNavmeshTilesUpdatedUntilFinishedDelegate, const TSet<uint32>&, ChangedTiles);

/**
 * Subclass of ARecastNavMesh that notifies listeners of Recast's tile updates via a custom delegate.
 */
UCLASS()
class COVERDEMO_API AChangeNotifyingRecastNavMesh : public ARecastNavMesh
{
	GENERATED_BODY()

protected:
	TSet<uint32> UpdatedTilesIntervalBuffer;

	TSet<uint32> UpdatedTilesUntilFinishedBuffer;

	// Lock used for interval-buffered tile updates.
	FCriticalSection TileUpdateLockObject;

	const float TileBufferInterval = 0.2f;

	FTimerHandle TileUpdateTimerHandle;
	
public:	
	UPROPERTY()
	FNavmeshTilesUpdatedImmediateDelegate NavmeshTilesUpdatedImmediateDelegate;

	UPROPERTY()
	FNavmeshTilesUpdatedBufferedDelegate NavmeshTilesUpdatedBufferedDelegate;

	UPROPERTY()
	FNavmeshTilesUpdatedUntilFinishedDelegate NavmeshTilesUpdatedUntilFinishedDelegate;

	AChangeNotifyingRecastNavMesh();

	AChangeNotifyingRecastNavMesh(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Called after a set of tiles had been updated. Due to how Recast's implementation works, it may repeatedly contain the same tiles between successive invocations.
	// This is worked around by buffering tile updates (see delegates).
	virtual void OnNavMeshTilesUpdated(const TArray<uint32>& ChangedTiles) override;

	// Broadcasts buffered tile updates every TileBufferInterval seconds via NavmeshTilesUpdatedDelegate. Thread-safe.
	UFUNCTION()
	void ProcessQueuedTiles();

	// Delegate handler.
	UFUNCTION()
	void OnNavmeshGenerationFinishedHandler(ANavigationData* NavData);
};
