// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "EquippableItem.generated.h"

//All the slots that gear can be equipped to.
UENUM(BlueprintType)
enum class EEquippableSlot : uint8
{
	EIS_Head UMETA(DisplayName = "Head"),
	EIS_Helmet UMETA(DisplayName = "Helmet"),
	EIS_Chest UMETA(DisplayName = "Chest"),
	EIS_Vest UMETA(DisplayName = "Vest"),
	EIS_Legs UMETA(DisplayName = "Legs"),
	EIS_Feet UMETA(DisplayName = "Feet"),
	EIS_Hands UMETA(DisplayName = "Hands"),
	EIS_Backpack UMETA(DisplayName = "Backpack"),
	EIS_PrimaryWeapon UMETA(DisplayName = "PrimaryWeapon"),
	EIS_SecondaryWeapon UMETA(DisplayName = "Secondary Weapon"), 
	EIS_Throwable UMETA(DisplayName = "Throwable Item")
};

/**
 * 
 */
UCLASS(Abstract, NotBlueprintable)
class SHOOTERPROJECT_API UEquippableItem : public UItem
{
	GENERATED_BODY()

public:

	UEquippableItem();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bItemDropped;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippables")
	EEquippableSlot Slot;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const override;

	virtual void Use(class AShooterProjectCharacter* Character) override;

	virtual void PlaceItem(class AShooterProjectCharacter* Character) override;

	UFUNCTION(BlueprintCallable, Category = "Equippables")
	virtual bool Equip(class AShooterProjectCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Equippables")
	virtual bool Unequip(class AShooterProjectCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Equippables")
	virtual bool Spawn(class AShooterProjectCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Equippables")
	virtual bool Remove(class AShooterProjectCharacter* Character);

	virtual bool ShouldShowInInventory() const override;

	UFUNCTION(BlueprintPure, Category = "Equippables")
	bool IsEquipped() { return bEquipped; };
	
	UFUNCTION(BlueprintPure, Category = "Equippables")
	bool IsSpawned() { return bSpawned; };

	void SetEquipped(bool bNewEquipped);
	void SetSpawned(bool bNewSpawn);

protected:

	
	//True if item is equipped, and false if unequipped
	UPROPERTY(ReplicatedUsing = EquipStatusChanged)
	bool bEquipped;

	//True if item is equipped, and false if unequipped
	UPROPERTY(ReplicatedUsing = EquipStatusChanged)
	bool bSpawned;
	
	UFUNCTION()
	void EquipStatusChanged();

	UFUNCTION()
	void SpawnStatusChanged();
	
};
