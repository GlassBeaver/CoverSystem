// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "CoverSystem/CoverSystem.h"
#include "CoverDemoGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class COVERDEMO_API ACoverDemoGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

protected:
	// This is only stored here to prevent the GC from destroying the cover system every now and then, nullifying the underlying octree.
	UPROPERTY()
	TObjectPtr<UCoverSystem> CoverSystem;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebugDraw = false;

	// Points to the AABB that is used to filter out cover points on the edges of the map.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBox MapBounds;

	ACoverDemoGameModeBase();

	virtual void PostLoad() override;

	virtual void PostActorCreated() override;

	//UFUNCTION()
	virtual void OnWorldInitDelegate(UWorld* World, const UWorld::InitializationValues IVS);

	// Used to inject our custom navmesh as an actor
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	virtual void PreInitializeComponents() override;

	virtual void BeginPlay() override;

	// [DEBUG] Displays all the cover points on the map.
	UFUNCTION(BlueprintCallable)
	void DebugShowCoverPoints();

	// [DEBUG] Forces garbage collection.
	// Useful for checking if singletons have a permanent reference to them, e.g. a UPROPERTY in game state.
	UFUNCTION(BlueprintCallable)
	void ForceGC();
};
