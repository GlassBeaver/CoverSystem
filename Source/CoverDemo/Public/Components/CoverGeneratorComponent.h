// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoverSystem/CoverSystem.h"
#include "CoverGeneratorComponent.generated.h"

// Component that generates CoverPoints at run-time as well as at design-time.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COVERDEMO_API UCoverGeneratorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCoverGeneratorComponent();

private:
	// Expand the bounding box of the object by this amount * ScanGridUnit, it's for when the navmesh around the object would exceed the object's bounds by too much.
	const float BoundingBoxExpansion = 0.5f;

#if DEBUG_RENDERING
	bool bDebugDraw = false;
#endif

	// Cached here for easy access on BeginDestroy()
	FBox OwnerBounds;

	// Stores whether the navmesh has already been generated at least once.
	bool bFirstNavmeshGeneration = true;

	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Should cover points be generated right at BeginPlay or not. Should be set to false for most ordnances. Need to call GenerateCoverPoints() later explicitly when this is set to false.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bGenerateOnBeginPlay = true;

	// Whether to generate cover points per UStaticMeshComponent found in Owner or around all the colliding components inside the owner's bounding box. Set to true if owner's bounding box would likely intersect with other actors in-game.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bGeneratePerStaticMesh = false;

	// Density of the scan grid (lower number -> more traces); Guideline: should be a bit less than the capsule radius of the smallest unit capable of getting into cover, which is normally == smallest radius used for navigation by the navmesh.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float ScanGridUnit = 75.0f;

	// Height of the smallest actor that will ever fit under an overhanging cover. Should normally be set to the crouched height of the smallest actor in the game.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float SmallestAgentHeight = 190.0f;

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	void OnNavmeshGenerationFinished(ANavigationData* NavData);

	// Generates cover points around (and inside) the owner via an asynchronous CoverPointGeneratorTask, which automatically stores the results in the game mode's octree.
	UFUNCTION(BlueprintCallable)
	void GenerateCoverPoints();
};
