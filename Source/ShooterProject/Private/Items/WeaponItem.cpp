// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/WeaponItem.h"

#include "Player/ShooterProjectCharacter.h"

UWeaponItem::UWeaponItem()
{
}

bool UWeaponItem::Equip(AShooterProjectCharacter* Character)
{
	bool BEquipSuccessful =  Super::Equip(Character);

	if (BEquipSuccessful && Character)
	{
		Character->EquipWeapon(this);
	}

	return BEquipSuccessful;
}

bool UWeaponItem::Unequip(AShooterProjectCharacter* Character)
{
	bool bUnEquipSuccessful = Super::Unequip(Character);

	if (bUnEquipSuccessful && Character)
	{
		Character->UnEquipWeapon();
	}

	return bUnEquipSuccessful;
}
