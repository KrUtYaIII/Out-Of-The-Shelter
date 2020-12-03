// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PickableItem.h"
#include "Cache_Key.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT9_API ACache_Key : public APickableItem
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		int KeyType;
};
