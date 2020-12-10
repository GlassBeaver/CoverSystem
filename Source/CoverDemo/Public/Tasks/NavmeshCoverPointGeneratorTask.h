// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "CoverSystem/CoverSystem.h"
#include "CoverSystem/DTOCoverData.h"

/**
 * Asynchronous, non-abandonable task for generating cover points and inserting them into an octree via UCoverSystem.
 */
class COVERDEMO_API FNavmeshCoverPointGeneratorTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FNavmeshCoverPointGeneratorTask>;

private:
	static FVector GetPerpendicularVector(const FVector& Vector);

	// Minimum distance between cover points.
	float CoverPointMinDistance;

	// How far to trace from a navmesh edge towards the inside of the navmesh "hole".
	const float ScanReach = 100.0f;

	// Distance between an outermost edge of the navmesh and a cliff, i.e. no navmesh.
	const float CliffEdgeDistance = 70.0f;

	// Offset that gets added to the cliff edge trace. Useful for detecting not perfectly straight cliffs e.g. that of landscapes.
	const float StraightCliffErrorTolerance = 100.0f;

	// Length of the raycast for checking if there's a navmesh hole to one of the sides of a navmesh edge.
	const float NavmeshHoleCheckReach = 5.0f;

	// Height of the smallest actor that will ever fit under an overhanging cover. Should normally be the CROUCHED height of the smallest actor in the game. Not counting bunnies. Bunnies are useless.
	const float SmallestAgentHeight;

	// A small Z-axis offset applied to each cover point. This is to prevent small irregularities in the navmesh from registering as cover.
	const float CoverPointGroundOffset;

	// Distance of the navmesh from the ground (UCoverSystem::CoverPointGroundOffset), plus a little extra to make sure we do hit the ground with the trace.
	// Used for finding cliff edges.
	const float NavMeshMaxZDistanceFromGround;

	// AABB used to filter out cover points on the edges of the map.
	const FBox MapBounds;

	// The bounding box to generate cover points in.
	const int32 NavmeshTileIndex;

	// The active world.
	UWorld* World;

#if DEBUG_RENDERING
	bool bDebugDraw = false;
#endif

	const FVector GetEdgeDir(const FVector& EdgeStartVertex, const FVector& EdgeEndVertex) const;

	// Uses navmesh raycasts to scan for cover from TraceStart to TraceEnd.
	// Builds an FDTOCoverData for transferring the results over to the cover octree.
	// Returns true if cover was found, false if not.
	bool ScanForCoverNavMeshProjection(FDTOCoverData& OutCoverData, const FVector& TraceStart, const FVector& TraceDirection);

	void ProcessEdgeStep(TArray<FDTOCoverData>& OutCoverPointsOfActors, const FVector& EdgeStepVertex, const FVector& EdgeDir);

	// Generates cover points inside the specified bounding box via navmesh edge-walking.
	// Returns the AABB of the navmesh tile that corresponds to NavmeshTileIndex.
	const FBox GenerateCoverInBounds(TArray<FDTOCoverData>& OutCoverPointsOfActors);

	// Find cover points in the supplied bounding box and store them in the cover system.
	// Remove any cover points within the supplied bounding box, first.
	void DoWork();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FNavmeshCoverPointGeneratorTask, STATGROUP_ThreadPoolAsyncTasks);
	}

public:
	FNavmeshCoverPointGeneratorTask(
		float _CoverPointMinDistance,
		float _SmallestAgentHeight,
		float _CoverPointGroundOffset,
		FBox _MapBounds,
		int32 _NavmeshTileIndex,
		UWorld* _World
	);
};
