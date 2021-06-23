// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/TargetPoint.h"
#include "ItemSpawn.generated.h"

USTRUCT(BlueprintType)
struct FLootTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:

	//The Item(s) to spawn
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TArray<TSubclassOf<class UItem>> Items;

	//The percentage chance of spawning this item if we hit it on the roll
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = 0.001, ClampMax = 1.0))
	float Probability = 1.f;
};

/**
 * 
 */
UCLASS()
class SHOOTERPROJECT_API AItemSpawn : public ATargetPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Loot")
	class UDataTable* LootTable;

	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TSubclassOf<class APickup> PickupClass;

	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	FIntPoint RespawnRange;

public:
	//Sets default values for this actor
	AItemSpawn();

protected:

	FTimerHandle TimerHandle_RespawnItem;

	UPROPERTY()
	TArray<AActor*> SpawnedPickups;

	//This is bound to the item being destroyed, so we can queue up another item to be spawned in
	UFUNCTION()
	void OnItemTaken(AActor* DestroyedActor);

	UFUNCTION()
	void SpawnItem();
	
	virtual void BeginPlay() override;
};
