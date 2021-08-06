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
	UseActionText = LOCTEXT("ItemUseActionText", "Equip");
}


void UEquippableItem::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Replicates bEquipped
	DOREPLIFETIME(UEquippableItem, bEquipped);
}

void UEquippableItem::Use(bool bInventoryOpen, class AShooterProjectCharacter* Character)
{
	if (Character && Character->HasAuthority())
	{
		if (Character->GetEquippedItems().Contains(Slot) && !bEquipped)
		{
			UEquippableItem* AlreadyEquippedItem = *Character->GetEquippedItems().Find(Slot);
			AlreadyEquippedItem->SetEquipped(false, bInventoryOpen);
		}

		SetEquipped(!IsEquipped(), bInventoryOpen);
	}
}

bool UEquippableItem::Equip(bool bInventoryOpen, class AShooterProjectCharacter* Character)
{
	if (Character)
	{
		return Character->EquipItem(bInventoryOpen, this);
	}
	return false;
}

bool UEquippableItem::Unequip(bool bInventoryOpen, class AShooterProjectCharacter* Character)
{
	if (Character)
	{
		return Character->UnEquipItem(bInventoryOpen,this);
	}
	return false;
}

bool UEquippableItem::ShouldShowInInventory() const
{
	return !bEquipped;
}

void UEquippableItem::SetEquipped(bool bNewEquipped, bool bInventoryOpen)
{
	bEquipped = bNewEquipped;
	EquipStatusChanged(bInventoryOpen);
	MarkDirtyForReplication();
}

void UEquippableItem::EquipStatusChanged(bool bInventoryOpen)
{
	if (AShooterProjectCharacter* Character = Cast<AShooterProjectCharacter>(GetOuter()))
	{
		if (bEquipped)
		{
			Equip(bInventoryOpen, Character);
		}
		else
		{
			Unequip(bInventoryOpen, Character);
		}
	}

	//Tell UI to update
	OnItemModified.Broadcast();
}

#undef LOCTEXT_NAMESPACE