// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DTOCoverData.h"

struct FCoverPointOctreeData : public TSharedFromThis<FCoverPointOctreeData, ESPMode::ThreadSafe>
{
public:
	// Location of the cover point
	const FVector Location;

	// no leaning if it's a force field wall

	// true if it's a force field, i.e. units can walk through but projectiles are blocked
	const bool bForceField;

	// Object that generated this cover point
	const TWeakObjectPtr<AActor> CoverObject;

	// Whether the cover point is taken by a unit
	bool bTaken = false;

	FCoverPointOctreeData()
		: Location(), bForceField(false), CoverObject(), bTaken(false)
	{}

	FCoverPointOctreeData(FDTOCoverData CoverData)
		: Location(CoverData.Location), bForceField(CoverData.bForceField), CoverObject(CoverData.CoverObject), bTaken(false)
	{}
};
