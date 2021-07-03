// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/GearItem.h"
#include "Player/ShooterProjectCharacter.h"

UGearItem::UGearItem()
{
	DamageDefenceMultiplier = 0.1f;
}

bool UGearItem::Equip(class AShooterProjectCharacter* Character)
{
	bool bEquipSuccessful = Super::Equip(Character);

	if (bEquipSuccessful && Character)
	{
		Character->EquipGear(this);
	}

	return bEquipSuccessful;
}

bool UGearItem::Unequip(class AShooterProjectCharacter* Character)
{
	bool bUnEquipSuccessful = Super::Unequip(Character);

	if (bUnEquipSuccessful && Character)
	{
		Character->UnEquipGear(Slot);
	}

	return bUnEquipSuccessful;
}
