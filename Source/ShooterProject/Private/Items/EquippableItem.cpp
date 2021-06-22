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
		return Character->UnequipItem(this);
	}
	return false;
}

bool UEquippableItem::ShouldShowInInventory() const
{
	return !bEquipped;
}

void UEquippableItem::SetEquipped(bool bNewEquipped)
{
	bEquipped = bNewEquipped;
	EquipStatusChanged();
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

#undef LOCTEXT_NAMESPACE