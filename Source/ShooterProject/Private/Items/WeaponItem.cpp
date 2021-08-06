// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/WeaponItem.h"

#include "Player/ShooterProjectCharacter.h"

UWeaponItem::UWeaponItem()
{
}

bool UWeaponItem::Equip(bool bInventoryOpen, AShooterProjectCharacter* Character)
{
	bool BEquipSuccessful =  Super::Equip(bInventoryOpen, Character);

	if (BEquipSuccessful && Character)
	{
		Character->EquipWeapon(bInventoryOpen, this);
	}

	return BEquipSuccessful;
}

bool UWeaponItem::Unequip(bool bInventoryOpen, AShooterProjectCharacter* Character)
{
	bool bUnEquipSuccessful = Super::Unequip(bInventoryOpen, Character);

	if (bUnEquipSuccessful && Character)
	{
		Character->UnEquipWeapon(bInventoryOpen, this);
	}

	return bUnEquipSuccessful;
}
