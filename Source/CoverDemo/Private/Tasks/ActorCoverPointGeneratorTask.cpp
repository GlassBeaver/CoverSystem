// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#include "Tasks/ActorCoverPointGeneratorTask.h"

#if DEBUG_RENDERING
#include "DrawDebugHelpers.h"
#endif

FActorCoverPointGeneratorTask::FActorCoverPointGeneratorTask(
	AActor* _Owner,
	UWorld* _World,
	float _BoundingBoxExpansion,
	float _ScanGridUnit,
	float _SmallestAgentHeight,
	bool _bGeneratePerStaticMesh)
	: Owner(_Owner),
	World(_World),
	BoundingBoxExpansion(_BoundingBoxExpansion),
	ScanGridUnit(_ScanGridUnit),
	SmallestAgentHeight(_SmallestAgentHeight),
	bGeneratePerStaticMesh(_bGeneratePerStaticMesh)
{}

const bool FActorCoverPointGeneratorTask::FindGroundPoint(FVector& OutGroundPoint, const FVector Location) const
{
	// trace downwards by grid size
	FHitResult hit;
	FCollisionQueryParams collQueryParams;
	collQueryParams.bFindInitialOverlaps;
	collQueryParams.AddIgnoredActor(Owner);
	collQueryParams.TraceTag = "CoverGenerator_FindGroundPoint";

	bool result = World->LineTraceSingleByChannel(hit, Location, Location - FVector(0.0f, 0.0f, ScanGridUnit), ECollisionChannel::ECC_GameTraceChannel1, collQueryParams);

	OutGroundPoint = hit.ImpactPoint;
	return result && !hit.bStartPenetrating;
}

void FActorCoverPointGeneratorTask::GatherFreeGridPoints(TArray<FVector>& OutGridPoints, const FVector BlockedGridPoint, const TArray<FVector>& FreeGridPoints, const float NavPointEqualityTolerance)
{
	for (const FVector& gridPoint : FreeGridPoints)
	{
		if (FMath::IsNearlyEqual(gridPoint.X, BlockedGridPoint.X, NavPointEqualityTolerance)
			&& FMath::IsNearlyEqual(gridPoint.Y, BlockedGridPoint.Y, NavPointEqualityTolerance)
			&& FMath::IsNearlyEqual(gridPoint.Z, BlockedGridPoint.Z, NavPointEqualityTolerance))
		{
			bool bUnique = true;
			for (const FVector& coverPoint : OutGridPoints)
				if (FMath::IsNearlyEqual(gridPoint.X, coverPoint.X, NavPointEqualityTolerance)
					&& FMath::IsNearlyEqual(gridPoint.Y, coverPoint.Y, NavPointEqualityTolerance)
					&& FMath::IsNearlyEqual(gridPoint.Z, coverPoint.Z, NavPointEqualityTolerance))
				{
					bUnique = false;
					break;
				}

			if (bUnique)
				OutGridPoints.Add(gridPoint);
		}
	}
}

void FActorCoverPointGeneratorTask::GenerateCoverInBounds(TArray<FDTOCoverData>& OutCoverPointsOfActors, FBox& Bounds)
{
	// profiling
	SCOPE_CYCLE_COUNTER(STAT_GenerateCoverInBounds);
	INC_DWORD_STAT(STAT_GenerateCoverHistoricalCount);
	SCOPE_SECONDS_ACCUMULATOR(STAT_GenerateCoverAverageTime);

	// expand the bounding box by a predetermined amount
	Bounds = Bounds.ExpandBy(ScanGridUnit * BoundingBoxExpansion);

	// arbitrarily set minimum cover height to be half of SmallestAgentHeight
	const float minCoverHeight = SmallestAgentHeight * 0.5f;
	const float boundsLengthX = Bounds.Max.X - Bounds.Min.X;
	const float boundsLengthY = Bounds.Max.Y - Bounds.Min.Y;
	const float boundsLengthZ = Bounds.Max.Z - Bounds.Min.Z;
	const int gridCountX = FMath::FloorToInt(boundsLengthX / ScanGridUnit) + 2;
	const int gridCountY = FMath::FloorToInt(boundsLengthY / ScanGridUnit) + 2;
	const int gridCountZ = FMath::FloorToInt(boundsLengthZ / ScanGridUnit) + 2;
	const FVector navProjectionExtent = FVector(ScanGridUnit * 5, ScanGridUnit * 5, ScanGridUnit * 1.5f);
	const float navPointEqualityTolerance = ScanGridUnit * 0.5f;

#if DEBUG_RENDERING
	if (bDebugDraw)
		DrawDebugBox(World, Bounds.GetCenter(), Bounds.GetExtent(), FColor::Orange, false, 1.0f);
#endif

	FHitResult hit;
	FCollisionQueryParams collQueryParams;
	collQueryParams.bFindInitialOverlaps = true;
	collQueryParams.TraceTag = "CoverGenerator_GenerateCoverPoints";

	TArray<FVector> freeGridPoints;
	TArray<FVector> blockedGridPoints;

	// divide Bounds into a 3D grid and iterate over all the grid points
	float traceX, traceY, traceZ;
	for (int x = 0; x < gridCountX; x++)
	{
		traceX = Bounds.Min.X + (x * ScanGridUnit);
		for (int y = 0; y < gridCountY; y++)
		{
			traceY = Bounds.Min.Y + (y * ScanGridUnit);
			for (int z = 0; z < gridCountZ; z++)
			{
				traceZ = Bounds.Min.Z + (z * ScanGridUnit);

				// find the ground
				FVector groundPoint;
				if (!FindGroundPoint(groundPoint, FVector(traceX, traceY, traceZ)))
					continue;
				groundPoint.Z += 1.0f; // otherwise we're starting the next ray from inside the ground

				// start location: ground position + minCoverHeight on the Z-axis
				// end location: ground position + SmallestAgentHeight on the Z-axis
				bool traceResult = World->LineTraceSingleByChannel(hit, groundPoint + FVector(0.0f, 0.0f, minCoverHeight), groundPoint + FVector(0.0f, 0.0f, SmallestAgentHeight), ECollisionChannel::ECC_GameTraceChannel1, collQueryParams);
				if (!traceResult && !hit.bStartPenetrating)
					// encountered a non-blocking hit
					freeGridPoints.Add(groundPoint);
				else
					// encountered a blocking hit
					blockedGridPoints.Add(groundPoint);
			}
		}
	}

	TArray<FVector> finalGridPoints;

	// find the nearest free grid points to each blocked grid point and project them onto the navmesh
	for (const FVector blockedGridPoint : blockedGridPoints)
		for (int x = -1; x <= 1; x++)
			for (int y = -1; y <= 1; y++)
				for (int z = -1; z <= 1; z++)
					GatherFreeGridPoints(finalGridPoints, FVector(blockedGridPoint.X + ScanGridUnit * x, blockedGridPoint.Y + ScanGridUnit * y, blockedGridPoint.Z + ScanGridUnit * z), freeGridPoints, navPointEqualityTolerance);

	// filter out any near-duplicates, i.e. vectors that are too close to one another
	FNavLocation navLocation;
	bool bUnique;
	for (FVector finalGridPoint : finalGridPoints)
	{
		if (UNavigationSystemV1::GetCurrent(World)->ProjectPointToNavigation(finalGridPoint, navLocation, navProjectionExtent))
		{
			bUnique = true;
			for (const FDTOCoverData coverPoint : OutCoverPointsOfActors)
				if (FMath::IsNearlyEqual(navLocation.Location.X, coverPoint.Location.X, navPointEqualityTolerance)
					&& FMath::IsNearlyEqual(navLocation.Location.Y, coverPoint.Location.Y, navPointEqualityTolerance)
					&& FMath::IsNearlyEqual(navLocation.Location.Z, coverPoint.Location.Z, navPointEqualityTolerance))
				{
					bUnique = false;
					break;
				}

			if (bUnique)
				OutCoverPointsOfActors.Add(FDTOCoverData(Owner, navLocation.Location, ECC_GameTraceChannel2 == Owner->GetRootComponent()->GetCollisionObjectType()));
		}
	}
}

void FActorCoverPointGeneratorTask::DoWork()
{
	// profiling
	SCOPE_CYCLE_COUNTER(STAT_GenerateCover);
	INC_DWORD_STAT(STAT_GenerateCoverHistoricalCount);
	SCOPE_SECONDS_ACCUMULATOR(STAT_GenerateCoverAverageTime);
	INC_DWORD_STAT(STAT_TaskCount);

	if (!IsValid(Owner))
		return;

#if DEBUG_RENDERING
	if (UCoverSystem::bShutdown)
		return;
	bDebugDraw = UCoverSystem::GetInstance(World)->bDebugDraw;
#endif

	TArray<FDTOCoverData> coverPoints;
	TArray<FBox> everyBoundingBox;

	if (bGeneratePerStaticMesh) // collect the bounding boxes of all the static meshes of Owner
	{
		TArray<UActorComponent*> staticMeshes = (Owner->GetComponentsByClass(UStaticMeshComponent::StaticClass()));
		for (UActorComponent* staticMesh : staticMeshes)
		{
			FBox bounds = Cast<UStaticMeshComponent>(staticMesh)->Bounds.GetBox();
			if (ScanGridUnit > bounds.Max.X - bounds.Min.X
				|| ScanGridUnit > bounds.Max.Y - bounds.Min.Y
				|| ScanGridUnit > bounds.Max.Z - bounds.Min.Z)
				continue;

			everyBoundingBox.Add(bounds);
		}
	}
	else // only need the Owner's bounding box
	{
		FBox bounds = Owner->GetComponentsBoundingBox();
		if (ScanGridUnit > bounds.Max.X - bounds.Min.X
			|| ScanGridUnit > bounds.Max.Y - bounds.Min.Y
			|| ScanGridUnit > bounds.Max.Z - bounds.Min.Z)
			return;

		everyBoundingBox.Add(bounds);
	}

	// generate cover using the bounding box(es)
	for (FBox boundingBox : everyBoundingBox)
		GenerateCoverInBounds(coverPoints, boundingBox);

	if (UCoverSystem::bShutdown)
		return;
	UCoverSystem::GetInstance(World)->AddCoverPoints(coverPoints);

	DEC_DWORD_STAT(STAT_TaskCount);
}
