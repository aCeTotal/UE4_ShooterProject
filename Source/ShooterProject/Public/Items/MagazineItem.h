// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/EquippableItem.h"
#include "MagazineItem.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERPROJECT_API UMagazineItem : public UEquippableItem
{
	GENERATED_BODY()

public:

	UMagazineItem();

//	virtual bool Equip(AShooterProjectCharacter* Character) override;
//	virtual bool Unequip(AShooterProjectCharacter* Character) override;

	//The Weapon class to give to the player upon equipping this weapon item
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<class AMagazine> MagazineClass;

	
};
