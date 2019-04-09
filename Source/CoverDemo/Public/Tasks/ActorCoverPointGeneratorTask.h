// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AsyncWork.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "NavigationSystem.h"
#include "CoverSystem/CoverSystem.h"
#include "CoverSystem/DTOCoverData.h"

/**
 * Asynchronous, non-abandonable task for generating cover points and inserting them into an octree via UCoverSystem.
 */
class COVERDEMO_API FActorCoverPointGeneratorTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FActorCoverPointGeneratorTask>;

protected:
	// The object to scan for cover points.
	AActor* Owner;

	// The active world.
	UWorld* World;

	// Expand the bounding box of the object by this amount * ScanGridUnit. It's for when the navmesh around the object would exceed the object's bounds by too much, resulting in no cover points being projected onto it.
	float BoundingBoxExpansion;

	// Density of the scan grid (lower number -> more traces); Guideline: should be a bit less than the capsule radius of the smallest unit capable of getting into cover, which is normally == smallest radius used for navigation by the navmesh.
	float ScanGridUnit;

	// Height of the smallest actor that will ever fit under an overhanging cover. Should normally be the CROUCHED height of the smallest actor in the game. Not counting bunnies. Bunnies are useless.
	float SmallestAgentHeight;

	// Whether to generate cover points per UStaticMeshComponent found in Owner or around all the colliding components inside the owner's bounding box. Set to true if owner's bounding box would likely intersect with other actors in-game.
	bool bGeneratePerStaticMesh;

#if DEBUG_RENDERING
	bool bDebugDraw = false;
#endif

	// Gets the nearest ground point to Location that's in one grid unit's range or less. Returns false if ground point was too far, i.e. more than a grid unit away. Does not use the navmesh.
	const bool FindGroundPoint(FVector& OutGroundPoint, const FVector Location) const;

	// Gathers all the free grid points around an blocked grid point and adds them to OutGridPoints. Checks in 8 directions.
	void GatherFreeGridPoints(TArray<FVector>& OutGridPoints, const FVector BlockedGridPoint, TArray<FVector>& FreeGridPoints, const float NavPointEqualityTolerance);

	// Generates cover points inside the specified bounding box. This method does the work.
	void GenerateCoverInBounds(TArray<FDTOCoverData>& OutCoverPointsOfActors, FBox& Bounds);

	// Find & store cover points in the game state. Calls GenerateCoverInBounds() either once when bGeneratePerStaticMesh == false or multiple times when bGeneratePerStaticMesh == true
	void DoWork();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FActorCoverPointGeneratorTask, STATGROUP_ThreadPoolAsyncTasks);
	}

public:
	FActorCoverPointGeneratorTask(
		AActor* _Owner,
		UWorld* _World,
		float _BoundingBoxExpansion,
		float _ScanGridUnit,
		float _SmallestAgentHeight,
		bool _bGeneratePerStaticMesh
	);
};
