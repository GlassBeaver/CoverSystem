// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#include "CoverSystem/CoverOctree.h"

TCoverOctree::TCoverOctree()
#if ENGINE_MINOR_VERSION < 26
	: TOctree<FCoverPointOctreeElement, FCoverPointOctreeSemantics>()
#else
	: TOctree2<FCoverPointOctreeElement, FCoverPointOctreeSemantics>()
#endif
{}

TCoverOctree::TCoverOctree(const FVector& Origin, float Radius)
#if ENGINE_MINOR_VERSION < 26
	: TOctree<FCoverPointOctreeElement, FCoverPointOctreeSemantics>(Origin, Radius)
#else
	: TOctree2<FCoverPointOctreeElement, FCoverPointOctreeSemantics>(Origin, Radius)
#endif
{}

TCoverOctree::~TCoverOctree()
{}

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
#if ENGINE_MINOR_VERSION < 26
	return (TCoverOctree::TConstElementBoxIterator<>(*this, QueryBox)).HasPendingElements();
#else
	bool result = false;
	FindFirstElementWithBoundsTest(QueryBox, [&result](const FCoverPointOctreeElement& CoverPoint) { result = true; return true; });
	return result;
#endif
}

void TCoverOctree::FindCoverPoints(TArray<FCoverPointOctreeElement>& OutCoverPoints, const FBox& QueryBox) const
{
#if ENGINE_MINOR_VERSION < 26
	for (TCoverOctree::TConstElementBoxIterator<> It(*this, QueryBox); It.HasPendingElements(); It.Advance())
		OutCoverPoints.Add(It.GetCurrentElement());
#else
	FindElementsWithBoundsTest(QueryBox, [&OutCoverPoints](const FCoverPointOctreeElement& CoverPoint) { OutCoverPoints.Add(CoverPoint); });
#endif
}

void TCoverOctree::FindCoverPoints(TArray<FCoverPointOctreeElement>& OutCoverPoints, const FSphere& QuerySphere) const
{
#if ENGINE_MINOR_VERSION < 26
	const FBoxCenterAndExtent& boxFromSphere = FBoxCenterAndExtent(QuerySphere.Center, FVector(QuerySphere.W));
	for (TCoverOctree::TConstElementBoxIterator<> It(*this, boxFromSphere); It.HasPendingElements(); It.Advance())
	{
		const FCoverPointOctreeElement& CoverPoint = It.GetCurrentElement();

		// check if cover point is inside the supplied sphere's radius, now that we've ballparked it with a box query
		if (QuerySphere.Intersects(CoverPoint.Bounds.GetSphere()))
			OutCoverPoints.Add(CoverPoint);
	}
#else
	const FBoxCenterAndExtent& boxFromSphere = FBoxCenterAndExtent(QuerySphere.Center, FVector(QuerySphere.W));
	FindElementsWithBoundsTest(boxFromSphere, [&OutCoverPoints, &QuerySphere](const FCoverPointOctreeElement& CoverPoint)
		{
			// check if cover point is inside the supplied sphere's radius, now that we've ballparked it with a box query
			if (QuerySphere.Intersects(CoverPoint.Bounds.GetSphere()))
				OutCoverPoints.Add(CoverPoint);
		});
#endif
}

#if ENGINE_MINOR_VERSION < 26
void TCoverOctree::RemoveElement(FOctreeElementId ElementID)
#else
void TCoverOctree::RemoveElement(FOctreeElementId2 ElementID)
#endif
{
	if (!ElementID.IsValidId())
		return;

#if ENGINE_MINOR_VERSION < 26
	static_cast<TOctree*>(this)->RemoveElement(ElementID);
#else
	static_cast<TOctree2*>(this)->RemoveElement(ElementID);
#endif
}

#if ENGINE_MINOR_VERSION < 26
bool TCoverOctree::HoldCover(FOctreeElementId ElementID)
#else
bool TCoverOctree::HoldCover(FOctreeElementId2 ElementID)
#endif
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

#if ENGINE_MINOR_VERSION < 26
bool TCoverOctree::ReleaseCover(FOctreeElementId ElementID)
#else
bool TCoverOctree::ReleaseCover(FOctreeElementId2 ElementID)
#endif
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
