// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#include "CoverSystem/CoverOctree.h"

TCoverOctree::TCoverOctree()
	: TOctree<FCoverPointOctreeElement, FCoverPointOctreeSemantics>()
{}

TCoverOctree::TCoverOctree(const FVector& Origin, float Radius)
	: TOctree<FCoverPointOctreeElement, FCoverPointOctreeSemantics>(Origin, Radius)
{}

TCoverOctree::~TCoverOctree()
{}

bool TCoverOctree::AddCoverPoint(FDTOCoverData& CoverData, const float DuplicateRadius)
{
	// check if any cover points are close enough - if so, abort
	if (AnyCoverPointsWithinBounds(FBoxCenterAndExtent(CoverData.Location, FVector(DuplicateRadius)).GetBox()))
		return false;

	AddElement(FCoverPointOctreeElement(CoverData));
	return true;
}

bool TCoverOctree::AnyCoverPointsWithinBounds(const FBox& QueryBox) const
{
	return (TCoverOctree::TConstElementBoxIterator<> (*this, QueryBox)).HasPendingElements();
}

void TCoverOctree::FindCoverPoints(TArray<FCoverPointOctreeElement>& OutCoverPoints, const FBox& QueryBox) const
{
	for (TCoverOctree::TConstElementBoxIterator<> It(*this, QueryBox); It.HasPendingElements(); It.Advance())
		OutCoverPoints.Add(It.GetCurrentElement());
}

void TCoverOctree::FindCoverPoints(TArray<FCoverPointOctreeElement>& OutCoverPoints, const FSphere& QuerySphere) const
{
	const FBoxCenterAndExtent& boxFromSphere = FBoxCenterAndExtent(QuerySphere.Center, FVector(QuerySphere.W));
	for (TCoverOctree::TConstElementBoxIterator<> It(*this, boxFromSphere); It.HasPendingElements(); It.Advance())
	{
		const FCoverPointOctreeElement& CoverPoint = It.GetCurrentElement();

		// check if cover point is inside the supplied sphere's radius, now that we've ballparked it with a box query
		if (QuerySphere.Intersects(CoverPoint.Bounds.GetSphere()))
			OutCoverPoints.Add(CoverPoint);
	}
}

void TCoverOctree::RemoveElement(FOctreeElementId ElementID)
{
	if (!ElementID.IsValidId())
		return;

	static_cast<TOctree*>(this)->RemoveElement(ElementID);
}


bool TCoverOctree::HoldCover(FOctreeElementId ElementID)
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

bool TCoverOctree::ReleaseCover(FOctreeElementId ElementID)
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
