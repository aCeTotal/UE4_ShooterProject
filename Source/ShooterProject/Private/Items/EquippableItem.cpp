// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/EquippableItem.h"
#include "Net/UnrealNetwork.h"
#include "Player/ShooterProjectCharacter.h"
#include "Components/InventoryComponent.h"

#define LOCTEXT_NAMESPACE "EquippableItem"

UEquippableItem::UEquippableItem()
{
	bStackable = false;
	bEquipped = false;
	bItemDropped = false;
	UseActionText = LOCTEXT("ItemUseActionText", "Equip");
}


void UEquippableItem::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Replicates bEquipped
	DOREPLIFETIME(UEquippableItem, bEquipped);
	DOREPLIFETIME(UEquippableItem, bSpawned);
}

void UEquippableItem::Use(class AShooterProjectCharacter* Character)
{
	if (Character && Character->HasAuthority())
	{
		if (Character->GetEquippedItems().Contains(Slot) && !bEquipped)
		{
			UEquippableItem* AlreadyEquippedItem = *Character->GetEquippedItems().Find(Slot);
			AlreadyEquippedItem->SetEquipped(false);
		}
		SetEquipped(!IsEquipped());
	}
}

void UEquippableItem::PlaceItem(AShooterProjectCharacter* Character)
{
	if (Character && Character->HasAuthority())
	{
		if (Character->GetEquippedItems().Contains(Slot) && !bSpawned)
		{
			UEquippableItem* AlreadyEquippedItem = *Character->GetEquippedItems().Find(Slot);
			AlreadyEquippedItem->SetSpawned(false);
		}

		SetSpawned(!IsSpawned());
	}
}

bool UEquippableItem::Equip(class AShooterProjectCharacter* Character)
{
	if (Character)
	{
		return Character->EquipItem(this);
	}
	return false;
}

bool UEquippableItem::Unequip(class AShooterProjectCharacter* Character)
{
	if (Character)
	{
		return Character->UnEquipItem(this);
	}
	return false;
}

bool UEquippableItem::Spawn(class AShooterProjectCharacter* Character)
{
	if (Character)
	{
		return Character->SpawnItem(this);
	}
	return false;
}

bool UEquippableItem::Remove(class AShooterProjectCharacter* Character)
{
	if (Character)
	{
		return Character->RemoveItem(this);
	}
	return false;
}

bool UEquippableItem::ShouldShowInInventory() const
{
	return !bEquipped && !bSpawned;
}

void UEquippableItem::SetEquipped(bool bNewEquipped)
{
	bEquipped = bNewEquipped;
	EquipStatusChanged();
	MarkDirtyForReplication();
}

void UEquippableItem::SetSpawned(bool bNewSpawn)
{
	bSpawned = bNewSpawn;
	SpawnStatusChanged();
	MarkDirtyForReplication();
}

void UEquippableItem::EquipStatusChanged()
{
	if (AShooterProjectCharacter* Character = Cast<AShooterProjectCharacter>(GetOuter()))
	{
		if (bEquipped)
		{
			Equip(Character);
		}
		else
		{
			Unequip(Character);
		}
	}

	//Tell UI to update
	OnItemModified.Broadcast();
}

void UEquippableItem::SpawnStatusChanged()
{
	if (AShooterProjectCharacter* Character = Cast<AShooterProjectCharacter>(GetOuter()))
	{
		if (bSpawned)
		{
			Spawn(Character);
		}
		else
		{
			Remove(Character);
		}
	}

	//Tell UI to update
	OnItemModified.Broadcast();
}

#undef LOCTEXT_NAMESPACE