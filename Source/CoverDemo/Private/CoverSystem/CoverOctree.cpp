// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#include "CoverSystem/CoverOctree.h"

TCoverOctree::TCoverOctree()

	: TOctree2<FCoverPointOctreeElement, FCoverPointOctreeSemantics>()
{
}

TCoverOctree::TCoverOctree(const FVector& Origin, float Radius)
	: TOctree2<FCoverPointOctreeElement, FCoverPointOctreeSemantics>(Origin, Radius)
{
}

TCoverOctree::~TCoverOctree()
{
}

bool TCoverOctree::AddCoverPoint(FDTOCoverData& CoverData, const float DuplicateRadius)
{
	// check if any cover points are close enough - if so, abort
	if (AnyCoverPointsWithinBounds(FBoxCenterAndExtent(CoverData.Location, FVector(DuplicateRadius))))
		return false;

	AddElement(FCoverPointOctreeElement(CoverData));
	return true;
}

bool TCoverOctree::AnyCoverPointsWithinBounds(const FBoxCenterAndExtent& QueryBox) const
{
	bool result = false;
	FindFirstElementWithBoundsTest(QueryBox, [&result](const FCoverPointOctreeElement& CoverPoint)
	{
		result = true;
		return true;
	});
	return result;
}

void TCoverOctree::FindCoverPoints(TArray<FCoverPointOctreeElement>& OutCoverPoints, const FBox& QueryBox) const
{
	FindElementsWithBoundsTest(QueryBox, [&OutCoverPoints](const FCoverPointOctreeElement& CoverPoint) { OutCoverPoints.Add(CoverPoint); });
}

void TCoverOctree::FindCoverPoints(TArray<FCoverPointOctreeElement>& OutCoverPoints, const FSphere& QuerySphere) const
{
	const FBoxCenterAndExtent& boxFromSphere = FBoxCenterAndExtent(QuerySphere.Center, FVector(QuerySphere.W));
	FindElementsWithBoundsTest(boxFromSphere, [&OutCoverPoints, &QuerySphere](const FCoverPointOctreeElement& CoverPoint)
	{
		// check if cover point is inside the supplied sphere's radius, now that we've ballparked it with a box query
		if (QuerySphere.Intersects(CoverPoint.Bounds.GetSphere()))
			OutCoverPoints.Add(CoverPoint);
	});
}


void TCoverOctree::RemoveElement(FOctreeElementId2 ElementID)
{
	if (!ElementID.IsValidId())
		return;


	static_cast<TOctree2*>(this)->RemoveElement(ElementID);
}


bool TCoverOctree::HoldCover(FOctreeElementId2 ElementID)
{
	if (!ElementID.IsValidId())
		return false;

	FCoverPointOctreeElement coverElement = GetElementById(ElementID);
	return HoldCover(coverElement);
}

bool TCoverOctree::HoldCover(FCoverPointOctreeElement Element)
{
	if (Element.Data->bTaken)
		return false;

	Element.Data->bTaken = true;
	return true;
}


bool TCoverOctree::ReleaseCover(FOctreeElementId2 ElementID)
{
	if (!ElementID.IsValidId())
		return false;

	FCoverPointOctreeElement coverElement = GetElementById(ElementID);
	return ReleaseCover(coverElement);
}

bool TCoverOctree::ReleaseCover(FCoverPointOctreeElement Element)
{
	if (!Element.Data->bTaken)
		return false;

	Element.Data->bTaken = false;
	return true;
}
