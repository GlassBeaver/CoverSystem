// Copyright (c) 2018 David Nadaski. All Rights Reserved.

#include "CoverSystem/CoverPointOctreeSemantics.h"
#include "CoverSystem/CoverSystem.h"

void FCoverPointOctreeSemantics::SetElementId(const FCoverPointOctreeElement& Element, FOctreeElementId ID)
{
	UWorld* world = nullptr;
	UObject* ElementOwner = Element.GetOwner();

	if (AActor* Actor = Cast<AActor>(ElementOwner))
	{
		world = Actor->GetWorld();
	}
	else if (UActorComponent* AC = Cast<UActorComponent>(ElementOwner))
	{
		world = AC->GetWorld();
	}
	else if (ULevel* Level = Cast<ULevel>(ElementOwner))
	{
		world = Level->OwningWorld;
	}

	if (UCoverSystem::bShutdown)
		return;
	UCoverSystem::GetInstance(world)->AssignIDToElement(Element.Data->Location, ID);
}
