// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreItem.h"

ACoreItem::ACoreItem() {
	Scene = CreateDefaultSubobject<USceneComponent>("Scene");
	Scene->SetupAttachment(RootComponent);

	Collision = CreateDefaultSubobject<UBoxComponent>("Collision");
	Collision->SetupAttachment(Scene);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	Mesh->SetupAttachment(Scene);
}

void ACoreItem::PickUpEventServer(AChel* Player) {
	PlayPickUpSound();
	Player->DoesHave[TypeItem] = true;
	Destroy();
}

void ACoreItem::PickUpEventClient(AChel * Player)
{
	Player->UserView->ArraySwitcher[TypeItem]->SetActiveWidgetIndex(1);
}
