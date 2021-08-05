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
		Character->UnEquipWeapon(this);
	}

	return bUnEquipSuccessful;
}


bool UWeaponItem::Spawn(AShooterProjectCharacter* Character)
{
	bool bSpawnSuccessful =  Super::Spawn(Character);

	if (bSpawnSuccessful && Character)
	{
		Character->SpawnWeapon(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cant spawn Weapon - WeaponItem"));
	}

	return bSpawnSuccessful;
}

bool UWeaponItem::Remove(AShooterProjectCharacter* Character)
{
	bool bRemovedSuccessful = Super::Remove(Character);

	if (bRemovedSuccessful && Character)
	{
		Character->RemoveWeapon(this);
	}

	return bRemovedSuccessful;
}
