// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * DTO for FCoverPointOctreeData
 */
struct FDTOCoverData
{
public:
	AActor* CoverObject;
	FVector Location;
	bool bForceField;

	FDTOCoverData()
		: CoverObject(), Location(), bForceField()
	{}

	FDTOCoverData(AActor* _CoverObject, FVector _Location, bool _bForceField)
		: CoverObject(_CoverObject), Location(_Location), bForceField(_bForceField)
	{}
};
