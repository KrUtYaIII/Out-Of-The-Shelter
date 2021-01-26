// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreItem_Dropped.h"
#include "Chel.h"

ACoreItem_Dropped::ACoreItem_Dropped() {
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	Mesh->SetupAttachment(RootComponent);

	Collision = CreateDefaultSubobject<UBoxComponent>("Collision");
	Collision->SetupAttachment(Mesh);
}

void ACoreItem_Dropped::PickUpEventServer(AChel* Player) {
	PlayPickUpSound();
	Player->DoesHave[TypeItem] = true;
	Destroy();
}

bool ACoreItem_Dropped::PickUpEventClient(AChel * Player)
{
	Player->UserView->ArraySwitcher[TypeItem]->SetActiveWidgetIndex(1);
	PlayPickUpSound();
	if (GetLocalRole() != ROLE_Authority)
		Destroy();
	return true;
}