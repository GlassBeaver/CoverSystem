// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "CoverPointOctreeElement.h"

struct FCoverPointOctreeSemantics
{
	enum { MaxElementsPerLeaf = 16 };
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	FORCEINLINE static const FBoxSphereBounds& GetBoundingBox(const FCoverPointOctreeElement& Element)
	{
		return Element.Bounds;
	}

	FORCEINLINE static bool AreElementsEqual(const FCoverPointOctreeElement& A, const FCoverPointOctreeElement& B)
	{
		//TODO: revisit this when introducing new properties to FCoverPointData
		return A.Bounds == B.Bounds;
	}

#if ENGINE_MINOR_VERSION < 26
	static void SetElementId(const FCoverPointOctreeElement& Element, FOctreeElementId ID);
#else
	static void SetElementId(const FCoverPointOctreeElement& Element, FOctreeElementId2 ID);
#endif
};
