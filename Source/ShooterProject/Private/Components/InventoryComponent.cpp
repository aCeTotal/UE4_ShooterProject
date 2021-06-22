// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Items/Item.h"

#define LOCTEXT_NAMESPACE "Inventory"


// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	SetIsReplicatedByDefault(true);
}


FItemAddResult UInventoryComponent::TryAddItem(class UItem* Item)
{
	return TryAddItem_Internal(Item);
}


FItemAddResult UInventoryComponent::TryAddItemFromClass(TSubclassOf<class UItem> ItemClass, const int32 Quanity)
{
	UItem* Item = NewObject<UItem>(GetOwner(), ItemClass);
	Item->SetQuantity(Quanity);
	return TryAddItem_Internal(Item);
}


int32 UInventoryComponent::ConsumeItem(class UItem* Item)
{
	if (Item)
	{
		ConsumeItem(Item, Item->GetQuantity());
	}

	return 0;
}


int32 UInventoryComponent::ConsumeItem(class UItem* Item, const int32 Quantity)
{
	if (GetOwner() && GetOwner()->HasAuthority() && Item)
	{
		const int32 RemoveQuantity = FMath::Min(Quantity, Item->GetQuantity());

		//We shouldn't have a negative amount of the item after the drop
		ensure(!(Item->GetQuantity() - RemoveQuantity < 0));

		//We now have zero of this item, remove it from the inventory
		Item->SetQuantity(Item->GetQuantity() - RemoveQuantity);

		if (Item->GetQuantity() <= 0)
		{
			RemoveItem(Item);
		}
		else
		{
			ClientRefreshInventory();
		}

		return RemoveQuantity;
	}

	return 0;
}


bool UInventoryComponent::RemoveItem(class UItem* Item)
{
	if (GetOwner() && GetOwner()->HasAuthority()) //If server
	{
		if (Item)
		{
			Items.RemoveSingle(Item);

			ReplicatedItemsKey++;

			return true;
		}
	}

	return false;
}


bool UInventoryComponent::HasItem(TSubclassOf<class UItem> ItemClass, const int32 Quantity /*= 1*/) const
{
	if (UItem* ItemToFind = FindItemByClass(ItemClass))
	{
		return ItemToFind->GetQuantity() >= Quantity;
	}

	return false;
}


UItem* UInventoryComponent::FindItem(class UItem* Item) const
{
	if (Item)
	{
		for (auto& InvItem : Items)
		{
			if (InvItem && InvItem->GetClass() == Item->GetClass())
			{
				return InvItem;
			}
		}
	}

	return nullptr;
}


UItem* UInventoryComponent::FindItemByClass(TSubclassOf<class UItem> ItemClass) const
{
	for (auto& InvItem : Items)
	{
		if (InvItem && InvItem->GetClass() == ItemClass)
		{
			return InvItem;
		}
	}
	return nullptr;
}


TArray<UItem*> UInventoryComponent::FindItemsByClass(TSubclassOf<class UItem> ItemClass) const
{
	TArray<UItem*> ItemsOfClass;

	for (auto& InvItem : Items)
	{
		if (InvItem && InvItem->GetClass()->IsChildOf(ItemClass))
		{
			ItemsOfClass.Add(InvItem);
		}
	}

	return ItemsOfClass;
}


float UInventoryComponent::GetCurrentWeight() const
{
	float Weight = 0.f;

	for (auto& Item : Items)
	{
		if (Item)
		{
			Weight += Item->GetStackWeight();
		}
	}

	return Weight;
}

void UInventoryComponent::SetWeightCapacity(const float NewWeightCapacity)
{
	WeightCapacity = NewWeightCapacity;
	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::SetCapacity(const int32 NewCapacity)
{
	Capacity = NewCapacity;
	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::ClientRefreshInventory_Implementation()
{
	OnInventoryUpdated.Broadcast();
}


void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, Items);
}


bool UInventoryComponent::ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	//Check if the array of items needs to replicate
	if (Channel->KeyNeedsToReplicate(0, ReplicatedItemsKey))
	{
		for (auto& Item : Items)
		{
			if (Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey))
			{
				bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
			}
		}
	}

	return bWroteSomething;
}


UItem* UInventoryComponent::AddItem(class UItem* Item)
{
	if (GetOwner() && GetOwner()->HasAuthority()) //IF Server
	{
		//Duplicates the object and creates a new one with the correct owner.
		UItem* NewItem = NewObject<UItem>(GetOwner(), Item->GetClass());
		NewItem->SetQuantity(Item->GetQuantity());
		NewItem->OwningInventory = this;
		NewItem->AddedToInventory(this);
		Items.Add(NewItem);
		NewItem->MarkDirtyForReplication();

		return NewItem;
	}

	return nullptr;
}


void UInventoryComponent::OnRep_Items()
{
	OnInventoryUpdated.Broadcast();
}


FItemAddResult UInventoryComponent::TryAddItem_Internal(class UItem* Item)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		//Amount to be added
		const int32 AddAmount = Item->GetQuantity();

		//Checks if Inventory is full
		if (Items.Num() + 1 > GetCapacity())
		{
			return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryCapacityFullText", "Couldn't add item to inventory. Inventory is full"));
		}

		//Items with a weight of zero don't require a weight check
		if (!FMath::IsNearlyZero(Item->Weight))
		{
			if (GetCurrentWeight() + Item->Weight > GetWeightCapacity())
			{
				return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryTooMuchWeightText", "Couldn't add item to Inventory. Carrying too much weight"));
			}
		}

		//If the item is stackable, check if we already have it and add it to their stack
		if (Item->bStackable)
		{
			//Somehow the items quantity went over the max stack size. This shouldn't ever happen
			ensure(Item->GetQuantity() <= Item->MaxStackSize);

			if (UItem* ExistingItem = FindItem(Item))
			{
				if (ExistingItem->GetQuantity() < ExistingItem->MaxStackSize)
				{
					//Find out how much of the item to add
					const int32 CapacityMaxAddAmount = ExistingItem->MaxStackSize - ExistingItem->GetQuantity();
					int32 ActualAddAmount = FMath::Min(AddAmount, CapacityMaxAddAmount);

					FText ErrorText = LOCTEXT("InventoryErrorText", "Couldn't add all of the item to your inventory");

					//Adjust based on how much weight we can carry
					if (!FMath::IsNearlyZero(Item->Weight))
					{
						//Find the maximum amount of the item we could take due to weight
						const int32 WeightMaxAddAmount = FMath::FloorToInt((WeightCapacity - GetCurrentWeight()) / Item->Weight);
						ActualAddAmount = FMath::Min(ActualAddAmount, WeightMaxAddAmount);

						if (ActualAddAmount < AddAmount)
						{
							ErrorText = FText::Format(LOCTEXT("InventoryTooMuchWeightText", "Couldn't add entire stack of {ItemName} to Inventory"), Item->ItemDisplayName);
						}
					}
					else if (ActualAddAmount < AddAmount)
					{
						//If the item weights none and we cant take it, then where was a capacity issue
						ErrorText = FText::Format(LOCTEXT("InventoryCapacityFullText", "Couldn't add entire stack of {ItemName} to Inventory. Inventory was full."), Item->ItemDisplayName);
					}

					if (ActualAddAmount <= 0)
					{
						return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryErrorText", "Couldn't add item to inventory"));
					}

					ExistingItem->SetQuantity(ExistingItem->GetQuantity() + ActualAddAmount);

					//If we somehow get more of the item than the max stack size when something is wrong with our math
					ensure(ExistingItem->GetQuantity() <= ExistingItem->MaxStackSize);

					if (ActualAddAmount < AddAmount)
					{
						return FItemAddResult::AddedSome(AddAmount, ActualAddAmount, ErrorText);
					}
					else
					{
						return FItemAddResult::AddedAll(AddAmount);
					}
				}
				else
				{
					return FItemAddResult::AddedNone(AddAmount, FText::Format(LOCTEXT("InventoryFullStackText", "Couldn't add {ItemName}. You already have a full stack of this item"), Item->ItemDisplayName));
				}
			}
			else
			{
				/**Since we don't have any of this item, we'll add the full stack*/
				AddItem(Item);

				return FItemAddResult::AddedAll(AddAmount);
			}
		}
		else //Item is not stackable
		{
			//Non-stackable should always have a quantity of 1
			ensure(Item->GetQuantity() == 1);

			AddItem(Item);

			return FItemAddResult::AddedAll(AddAmount);
		}
	}
	
	//AddItem should never be called on a client
	check(false);
	return FItemAddResult::AddedNone(-1, LOCTEXT("ErrorMessage", ""));
}

#undef LOCTEXT_NAMESPACE

