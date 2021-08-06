// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/EquippableItem.h"
#include "GearItem.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class SHOOTERPROJECT_API UGearItem : public UEquippableItem
{
	GENERATED_BODY()

public:

	UGearItem();

	virtual bool Equip(bool bInventoryOpen, class AShooterProjectCharacter* Character) override;
	virtual bool Unequip(bool bInventoryOpen, class AShooterProjectCharacter* Character) override;

	/**The skeletal mesh for this gear*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gear")
	class USkeletalMesh* Mesh;

	/**Optional material instance to apply to the gear*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gear")
	class UMaterialInstance* MaterialInstance;

	/**The amount of defence this item provides. 0.2 = 20% less damage*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gear", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float DamageDefenceMultiplier;
};
