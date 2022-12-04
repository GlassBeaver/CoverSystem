// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/GenericOctreePublic.h"
#include "Math/GenericOctree.h"
#include "GameFramework/Actor.h"
#include "CoverPointOctreeElement.h"
#include "CoverPointOctreeSemantics.h"
#include "DTOCoverData.h"

/**
 * Octree for storing cover points. Not thread-safe, use UCoverSystem for manipulation.
 */

class TCoverOctree : public TOctree2<FCoverPointOctreeElement, FCoverPointOctreeSemantics>, public TSharedFromThis<TCoverOctree, ESPMode::ThreadSafe>
{
public:
	TCoverOctree();

	TCoverOctree(const FVector& Origin, float Radius);

	virtual ~TCoverOctree();

	// Adds a cover point to the octree.
	bool AddCoverPoint(FDTOCoverData& CoverData, const float DuplicateRadius);

	// Checks if any cover points are within the supplied bounds.
	bool AnyCoverPointsWithinBounds(const FBoxCenterAndExtent& QueryBox) const;

	// Finds cover points that intersect the supplied box.
	void FindCoverPoints(TArray<FCoverPointOctreeElement>& OutCoverPoints, const FBox& QueryBox) const;

	// Finds cover points that intersect the supplied sphere.
	void FindCoverPoints(TArray<FCoverPointOctreeElement>& OutCoverPoints, const FSphere& QuerySphere) const;

	// Won't crash the game if ElementID is invalid, unlike the similarly named superclass method. This method hides the base class method as it's not virtual.
	void RemoveElement(FOctreeElementId2 ElementID);

	// Mark the cover at the supplied location as taken.
	// Returns true if the cover wasn't already taken, false if it was or an error has occurred, e.g. the cover no longer exists.
	bool HoldCover(FOctreeElementId2 ElementID);
	bool HoldCover(FCoverPointOctreeElement Element);
	
	// Releases a cover that was taken.
	// Returns true if the cover was taken before, false if it wasn't or an error has occurred, e.g. the cover no longer exists.
	bool ReleaseCover(FOctreeElementId2 ElementID);
	bool ReleaseCover(FCoverPointOctreeElement Element);
};
