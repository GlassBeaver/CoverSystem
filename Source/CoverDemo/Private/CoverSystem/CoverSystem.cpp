// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#include "CoverSystem.h"
#include "Tasks/NavmeshCoverPointGeneratorTask.h"

#if DEBUG_RENDERING
#include "DrawDebugHelpers.h"
#endif

// PROFILER INTEGRATION //
DEFINE_STAT(STAT_GenerateCover);
DEFINE_STAT(STAT_GenerateCoverInBounds);
DEFINE_STAT(STAT_FindCover);

UCoverSystem* UCoverSystem::MyInstance;
bool UCoverSystem::bShutdown;

UCoverSystem::UCoverSystem()
{
	//TODO: take the extents of the underlying navigation mesh instead of using 64000, see NavData->GetBounds() in OnNavmeshUpdated
	CoverOctree = MakeShareable(new TCoverOctree(FVector(0, 0, 0), 64000));
}

UCoverSystem::~UCoverSystem()
{
	if (CoverOctree.IsValid())
	{
		CoverOctree->Destroy();
		CoverOctree = nullptr;
	}

	ElementToID.Empty();
	CoverObjectToID.Empty();
	MyInstance = nullptr;
}

UCoverSystem* UCoverSystem::GetInstance(UWorld* World)
{
	if (bShutdown)
		return nullptr;

	if (!IsValid(MyInstance))
	{
		// initialize the singleton
		MyInstance = NewObject<UCoverSystem>(World, "CoverSystem");
		check(MyInstance);
		MyInstance->OnBeginPlay();

		// reset the persistent profiler stats
		SET_DWORD_STAT(STAT_GenerateCoverHistoricalCount, 0);
		SET_FLOAT_STAT(STAT_GenerateCoverAverageTime, 0.0f);
		SET_DWORD_STAT(STAT_FindCoverHistoricalCount, 0);
		SET_FLOAT_STAT(STAT_FindCoverTotalTimeSpent, 0.0f);
		SET_DWORD_STAT(STAT_TaskCount, 0);
	}

	return MyInstance;
}

UCoverSystem* UCoverSystem::GetCoverSystem(UObject* WorldContextObject)
{
	UWorld* world = nullptr;
	if (WorldContextObject)
		world = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	return world ? GetInstance(world) : nullptr;
}

void UCoverSystem::OnBeginPlay()
{
	bShutdown = false;

	UNavigationSystemV1* navsys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!IsValid(navsys))
		return;

	// subscribe to tile update events on the navmesh
	const ANavigationData* mainNavData = navsys->MainNavData;
	if (mainNavData && mainNavData->IsA(AChangeNotifyingRecastNavMesh::StaticClass()))
	{
		Navmesh = const_cast<AChangeNotifyingRecastNavMesh*>(Cast<AChangeNotifyingRecastNavMesh>(mainNavData));
		Navmesh->NavmeshTilesUpdatedBufferedDelegate.AddDynamic(this, &UCoverSystem::OnNavMeshTilesUpdated);
	}

}

bool ContainsCoverPoint(FCoverPointOctreeElement CoverPoint, TArray<FCoverPointOctreeElement> CoverPoints)
{
	for (FCoverPointOctreeElement cp : CoverPoints)
		if (CoverPoint.Data->Location == cp.Data->Location)
			return true;

	return false;
}

bool UCoverSystem::GetElementID(FOctreeElementId& OutElementID, const FVector ElementLocation) const
{
	if (bShutdown)
		return false;

	const FOctreeElementId* element = ElementToID.Find(ElementLocation);
	if (!element || !element->IsValidId())
		return false;

	OutElementID = *element;
	return true;
}

void UCoverSystem::AssignIDToElement(const FVector ElementLocation, FOctreeElementId ID)
{
	if (bShutdown)
		return;

	ElementToID.Add(ElementLocation, ID);
}

bool UCoverSystem::RemoveIDToElementMapping(const FVector ElementLocation)
{
	if (bShutdown)
		return false;

	return ElementToID.Remove(ElementLocation) > 0;
}

void UCoverSystem::OnNavMeshTilesUpdated(const TSet<uint32>& UpdatedTiles)
{
	if (bShutdown)
		return;

	// regenerate cover points within the updated navmesh tiles
	TArray<const AActor*> dirtyCoverActors;
	for (uint32 tileIdx : UpdatedTiles)
	{
		// get the bounding box of the updated tile
		FBox tileBounds = Navmesh->GetNavMeshTileBounds(tileIdx);

#if DEBUG_RENDERING
		// DrawDebugXXX calls may crash UE4 when not called from the main thread, so start synchronous tasks in case we're planning on drawing debug shapes
		if (bDebugDraw)
			(new FAutoDeleteAsyncTask<FNavmeshCoverPointGeneratorTask>(
				CoverPointMinDistance,
				SmallestAgentHeight,
				CoverPointGroundOffset,
				MapBounds,
				tileIdx,
				GetWorld()
				))->StartSynchronousTask();
		else
#endif
			(new FAutoDeleteAsyncTask<FNavmeshCoverPointGeneratorTask>(
				CoverPointMinDistance,
				SmallestAgentHeight,
				CoverPointGroundOffset,
				MapBounds,
				tileIdx,
				GetWorld()
				))->StartBackgroundTask();
	}
}

void UCoverSystem::FindCoverPoints(TArray<FCoverPointOctreeElement>& OutCoverPoints, const FBox& QueryBox) const
{
	if (bShutdown)
		return;

	FRWScopeLock CoverDataLock(CoverDataLockObject, FRWScopeLockType::SLT_ReadOnly);
	CoverOctree->FindCoverPoints(OutCoverPoints, QueryBox);
}

void UCoverSystem::FindCoverPoints(TArray<FCoverPointOctreeElement>& OutCoverPoints, const FSphere& QuerySphere) const
{
	if (bShutdown)
		return;

	FRWScopeLock CoverDataLock(CoverDataLockObject, FRWScopeLockType::SLT_ReadOnly);
	CoverOctree->FindCoverPoints(OutCoverPoints, QuerySphere);
}

void UCoverSystem::AddCoverPoints(const TArray<FDTOCoverData>& CoverPointDTOs)
{
	if (bShutdown)
		return;

	FRWScopeLock CoverDataLock(CoverDataLockObject, FRWScopeLockType::SLT_Write);

	for (FDTOCoverData coverPointDTO : CoverPointDTOs)
		CoverOctree->AddCoverPoint(coverPointDTO, CoverPointMinDistance * 0.9f);

	// optimize the octree
	CoverOctree->ShrinkElements();
}

FBox UCoverSystem::EnlargeAABB(FBox Box)
{
	return Box.ExpandBy(FVector(
		(Box.Max.X - Box.Min.X) * 0.5f,
		(Box.Max.Y - Box.Min.Y) * 0.5f,
		(Box.Max.Z - Box.Min.Z) * 0.5f
	));
}

void UCoverSystem::RemoveStaleCoverPoints(FBox Area)
{
	if (bShutdown)
		return;

	FRWScopeLock CoverDataLock(CoverDataLockObject, FRWScopeLockType::SLT_Write);

	// enlarge the clean-up area to x1.5 its size
	Area = EnlargeAABB(Area);

	// find all the cover points in the specified area
	TArray<FCoverPointOctreeElement> coverPoints;
	CoverOctree->FindCoverPoints(coverPoints, Area);

	for (FCoverPointOctreeElement coverPoint : coverPoints)
	{
		// check if the cover point still has an owner and still falls on the exact same location on the navmesh as it did when it was generated
		FNavLocation navLocation;
		if (IsValid(coverPoint.GetOwner())
			&& UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(coverPoint.Data->Location, navLocation, FVector(0.1f, 0.1f, CoverPointGroundOffset)))
			continue;

		FOctreeElementId id;
		GetElementID(id, coverPoint.Data->Location);

		// remove the cover point from the octree
		if (id.IsValidId())
			CoverOctree->RemoveElement(id);

		// remove the cover point from the element-to-id and object-to-location maps
		RemoveIDToElementMapping(coverPoint.Data->Location);
		CoverObjectToID.RemoveSingle(coverPoint.Data->CoverObject, coverPoint.Data->Location);
	}

	// optimize the octree
	CoverOctree->ShrinkElements();
}

void UCoverSystem::RemoveStaleCoverPoints(FVector Origin, FVector Extent)
{
	if (bShutdown)
		return;

	RemoveStaleCoverPoints(FBoxCenterAndExtent(Origin, Extent * 2.0f).GetBox());
}

void UCoverSystem::RemoveCoverPointsOfObject(const AActor* CoverObject)
{
	if (bShutdown)
		return;

	FRWScopeLock CoverDataLock(CoverDataLockObject, FRWScopeLockType::SLT_Write);

	TArray<FVector> coverPointLocations;
	CoverObjectToID.MultiFind(CoverObject, coverPointLocations, false);

	for (const FVector coverPointLocation : coverPointLocations)
	{
		FOctreeElementId elementID;
		GetElementID(elementID, coverPointLocation);
		CoverOctree->RemoveElement(elementID);
		RemoveIDToElementMapping(coverPointLocation);
		CoverObjectToID.Remove(CoverObject);

#if DEBUG_RENDERING
		if (bDebugDraw)
			DrawDebugSphere(GetWorld(), coverPointLocation, 20.0f, 4, FColor::Red, true, -1.0f, 0, 2.0f);
#endif
	}

	// optimize the octree
	CoverOctree->ShrinkElements();
}

void UCoverSystem::RemoveAll()
{
	if (bShutdown)
		return;

	FRWScopeLock CoverDataLock(CoverDataLockObject, FRWScopeLockType::SLT_Write);

	// destroy the octree
	if (CoverOctree.IsValid())
	{
		CoverOctree->Destroy();
		CoverOctree = nullptr;
	}

	// remove the id-to-element mappings
	ElementToID.Empty();

	// make a new octree
	CoverOctree = MakeShareable(new TCoverOctree(FVector(0, 0, 0), 64000));
}

bool UCoverSystem::HoldCover(FVector ElementLocation)
{
	if (bShutdown)
		return false;

	FRWScopeLock CoverDataLock(CoverDataLockObject, FRWScopeLockType::SLT_Write);

	FOctreeElementId elemID;
	if (!GetElementID(elemID, ElementLocation))
		return false;

	return CoverOctree->HoldCover(elemID);
}

bool UCoverSystem::ReleaseCover(FVector ElementLocation)
{
	if (bShutdown)
		return false;

	FRWScopeLock CoverDataLock(CoverDataLockObject, FRWScopeLockType::SLT_Write);

	FOctreeElementId elemID;
	if (!GetElementID(elemID, ElementLocation))
		return false;

	return CoverOctree->ReleaseCover(elemID);
}
