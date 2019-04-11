// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#include "Tasks/NavmeshCoverPointGeneratorTask.h"
#include "LandscapeProxy.h"
#include "NavMesh/RecastNavMesh.h"
#include "Detour/DetourNavMesh.h"

#if DEBUG_RENDERING
#include "DrawDebugHelpers.h"
#endif

FNavmeshCoverPointGeneratorTask::FNavmeshCoverPointGeneratorTask(
	float _CoverPointMinDistance,
	float _SmallestAgentHeight,
	float _CoverPointGroundOffset,
	FBox _MapBounds,
	int32 _NavmeshTileIndex,
	UWorld* _World)
	: CoverPointMinDistance(_CoverPointMinDistance),
	SmallestAgentHeight(_SmallestAgentHeight),
	CoverPointGroundOffset(_CoverPointGroundOffset),
	NavMeshMaxZDistanceFromGround(_CoverPointGroundOffset * 3.0f),
	MapBounds(_MapBounds),
	NavmeshTileIndex(_NavmeshTileIndex),
	World(_World)
{}

FVector FNavmeshCoverPointGeneratorTask::GetPerpendicularVector(const FVector& Vector)
{
	return FVector(Vector.Y, -Vector.X, Vector.Z);
}

//TODO: consider using NavData.Raycast or NavData.BatchRaycast() instead
//TODO: compare raycast() to projectpointonnavigation() in terms of performance: run both alternatives on the same set of geo, on the same map
bool FNavmeshCoverPointGeneratorTask::ScanForCoverNavMeshProjection(FDTOCoverData& OutCoverData, const FVector& TraceStart, const FVector& TraceDirection)
{
	const FVector traceEndNavMesh = TraceStart + (TraceDirection * NavmeshHoleCheckReach); // need to keep the navmesh raycast as short as possible
	const FVector traceEndPhysX = TraceStart + (TraceDirection * ScanReach); // the physx raycast is longer so that it may reach slanted geometry, e.g. ramps
	const FVector smallestAgentHeightOffset = FVector(0.0f, 0.0f, SmallestAgentHeight);

	// project the point onto the navmesh: if the projection is successful then it's not a navmesh hole, so we return failure
	FNavLocation navLocation;
	if (UNavigationSystemV1::GetCurrent(World)->ProjectPointToNavigation(traceEndNavMesh, navLocation, FVector(0.1f, 0.1f, 0.1f)))
		return false;

	// to get the cover object within the hole in the navmesh we still need to do a raycast towards its general direction, at a height of SmallestAgentHeight to ensure that the cover is tall enough
	FHitResult hit;
	FCollisionQueryParams collQueryParams;
	collQueryParams.TraceTag = "CoverGenerator_ScanForCoverNavMeshProjection";

	bool bSuccess = World->LineTraceSingleByChannel(hit, TraceStart + smallestAgentHeightOffset, traceEndPhysX + smallestAgentHeightOffset, ECollisionChannel::ECC_GameTraceChannel1, collQueryParams);

	//TODO: comment out if not needed - ledge detection logic
	if (!bSuccess)
	{
		// if we didn't hit an object with the XY-parallel ray then cast another one towards the ground from an extended location, down along the Z-axis
		// this ensures that we pick up the edges of cliffs, which are valid cover points against units below, while also discarding flat planes that tend to creep up along navmesh tile boundaries
		// if this ray doesn't hit anything then we've found a cliff's edge
		// if it hits, then cast another, similar, but slightly slanted ray to account for any non-perfectly straight cliff walls e.g. that of landscapes
		FHitResult hitCliff;
		const FVector cliffTraceStart = traceEndNavMesh + (TraceDirection * CliffEdgeDistance);
		bSuccess = !World->LineTraceSingleByChannel(hitCliff, cliffTraceStart, cliffTraceStart - smallestAgentHeightOffset, ECollisionChannel::ECC_GameTraceChannel1, collQueryParams);

		if (!bSuccess)
			// this is the slightly slanted, 2nd ray
			bSuccess = !World->LineTraceSingleByChannel(hitCliff, cliffTraceStart, cliffTraceStart - smallestAgentHeightOffset + (TraceDirection * StraightCliffErrorTolerance), ECollisionChannel::ECC_GameTraceChannel1, collQueryParams);

		if (bSuccess)
			// now that we've established that it's a cliff's edge, we need to trace into the ground to find the "cliff object"
			bSuccess = World->LineTraceSingleByChannel(hit, TraceStart, TraceStart - FVector(0.0f, 0.0f, NavMeshMaxZDistanceFromGround), ECollisionChannel::ECC_GameTraceChannel1, collQueryParams);

	}

	if (!bSuccess)
		return false;

	// force fields (shields) are handled by FActorCoverPointGeneratorTask instead
	if (ECC_GameTraceChannel2 == hit.Actor->GetRootComponent()->GetCollisionObjectType())
		return false;

	OutCoverData = FDTOCoverData(hit.Actor.Get(), TraceStart, false);
	return true;
}

void FNavmeshCoverPointGeneratorTask::ProcessEdgeStep(TArray<FDTOCoverData>& OutCoverPointsOfActors, const FVector& EdgeStepVertex, const FVector& EdgeDir)
{
	if (!MapBounds.IsInside(EdgeStepVertex))
		return;

	FDTOCoverData coverData;
	for (int directionMultiplier = 1; directionMultiplier > -2; directionMultiplier -= 2)
		// check if we're at the edge of the map
		if (ScanForCoverNavMeshProjection(coverData, EdgeStepVertex, GetPerpendicularVector(EdgeDir) * directionMultiplier))
		{
			OutCoverPointsOfActors.Add(coverData);
			return;
		}
}

const FVector FNavmeshCoverPointGeneratorTask::GetEdgeDir(const FVector& EdgeStartVertex, const FVector& EdgeEndVertex) const
{
	return (EdgeEndVertex - EdgeStartVertex).GetUnsafeNormal();
}

const FBox FNavmeshCoverPointGeneratorTask::GenerateCoverInBounds(TArray<FDTOCoverData>& OutCoverPointsOfActors)
{
	// profiling
	SCOPE_CYCLE_COUNTER(STAT_GenerateCoverInBounds);
	INC_DWORD_STAT(STAT_GenerateCoverHistoricalCount);
	SCOPE_SECONDS_ACCUMULATOR(STAT_GenerateCoverAverageTime);

	const ARecastNavMesh* navdata = Cast<ARecastNavMesh>(UNavigationSystemV1::GetCurrent(World)->MainNavData);
	FRecastDebugGeometry navGeo;
	navGeo.bGatherNavMeshEdges = true;

	// get the navigation vertices from recast via a batch query
	navdata->BeginBatchQuery();
	navdata->GetDebugGeometry(navGeo, NavmeshTileIndex);
	navdata->FinishBatchQuery();

	// process the navmesh vertices (called nav mesh edges for some occult reason)
	TArray<FVector>& vertices = navGeo.NavMeshEdges;
	const int nVertices = vertices.Num();
	if (nVertices > 1)
	{
		const FVector firstEdgeDir = GetEdgeDir(vertices[0], vertices[1]);
		const FVector lastEdgeDir = GetEdgeDir(vertices[nVertices - 2], vertices[nVertices - 1]);
		for (int iVertex = 0; iVertex < nVertices; iVertex += 2)
		{
			const FVector edgeStartVertex = vertices[iVertex];
			const FVector edgeEndVertex = vertices[iVertex + 1];
			const FVector edge = edgeEndVertex - edgeStartVertex;
			const FVector edgeDir = edge.GetUnsafeNormal();

#if DEBUG_RENDERING
			if (bDebugDraw)
				DrawDebugDirectionalArrow(World, edgeStartVertex, edgeEndVertex, 200.0f, FColor::Purple, true, -1.0f, 0, 2.0f);
#endif

			// step through the edge in CoverPointMinDistance increments
			const int nEdgeSteps = edge.Size() / CoverPointMinDistance;
			for (int iEdgeStep = 0; iEdgeStep < nEdgeSteps; iEdgeStep++)
				// check to the left and to optionally, to the right of the vertex's location for any blocking geometry
				// if geometry blocks the raycast then the vertex is marked as a cover point
				ProcessEdgeStep(OutCoverPointsOfActors, edgeStartVertex + (iEdgeStep * CoverPointMinDistance * edgeDir) + FVector(0.0f, 0.0f, CoverPointGroundOffset), edgeDir, true);

			// process the first step if the edge was shorter than CoverPointMinDistance
			if (nEdgeSteps == 0)
				ProcessEdgeStep(OutCoverPointsOfActors, edgeStartVertex + FVector(0.0f, 0.0f, CoverPointGroundOffset), edgeDir, false);

			// process the end vertex; 99% of the time it's left out by the above for-loop, and in that 1% of cases we will just process the same vertex twice (likely to never happen because of floating-point division)
			ProcessEdgeStep(OutCoverPointsOfActors, edgeEndVertex + FVector(0.0f, 0.0f, CoverPointGroundOffset), edgeDir, false);

			// process the end vertex again, this time with its edge direction rotated by 45 degrees
			ProcessEdgeStep(OutCoverPointsOfActors, edgeEndVertex + FVector(0.0f, 0.0f, CoverPointGroundOffset), FVector(FVector2D(edgeDir).GetRotated(45.0f), edgeDir.Z), false);
		}
	}

	// return the AABB of the navmesh tile that's been processed, expanded by minimum tile height on the Z-axis
	const ARecastNavMesh* recastNavmesh = Cast<ARecastNavMesh>(UNavigationSystemV1::GetCurrent(World)->MainNavData);
	FBox navmeshBounds = navdata->GetNavMeshTileBounds(NavmeshTileIndex);
	float navmeshTileHeight = recastNavmesh->GetRecastMesh()->getParams()->tileHeight;
	if (navmeshTileHeight > 0)
		navmeshBounds = navmeshBounds.ExpandBy(FVector(0.0f, 0.0f, navmeshTileHeight * 0.5f));

	return navmeshBounds;
}

void FNavmeshCoverPointGeneratorTask::DoWork()
{
	// profiling
	SCOPE_CYCLE_COUNTER(STAT_GenerateCover);
	INC_DWORD_STAT(STAT_GenerateCoverHistoricalCount);
	SCOPE_SECONDS_ACCUMULATOR(STAT_GenerateCoverAverageTime);
	INC_DWORD_STAT(STAT_TaskCount);

#if DEBUG_RENDERING
	if (UCoverSystem::bShutdown)
		return;
	bDebugDraw = UCoverSystem::GetInstance(World)->bDebugDraw;
#endif

	// generate cover points
	TArray<FDTOCoverData> coverPoints;
	FBox navmeshTileArea = GenerateCoverInBounds(coverPoints);

	// remove any cover points that don't fall on the navmesh anymore
	// happens when a newly placed cover object is placed on top of previously generated cover points
	if (UCoverSystem::bShutdown)
		return;
	//TODO: consider deleting this - a few more cover points might be left over upon object removal but at the expense of fewer cover points per object. most apparent near ledges. not a big deal either way, though.
	UCoverSystem::GetInstance(World)->RemoveStaleCoverPoints(navmeshTileArea);

	// add the generated cover points to the octree in a single batch
	if (UCoverSystem::bShutdown)
		return;
	UCoverSystem::GetInstance(World)->AddCoverPoints(coverPoints);

#if DEBUG_RENDERING
	for (FDTOCoverData coverPoint : coverPoints)
		if (bDebugDraw)
			DrawDebugSphere(World, coverPoint.Location, 20.0f, 4, FColor::Blue, true);
#endif

	DEC_DWORD_STAT(STAT_TaskCount);
}
