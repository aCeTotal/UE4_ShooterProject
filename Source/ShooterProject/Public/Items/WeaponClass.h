// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponClass.generated.h"

class USkeletalMeshComponent;


UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	IDLE				UMETA(DisplayName = "Idle"),
	FIRING				UMETA(DisplayName = "Firing"),
	RELOADING			UMETA(DisplayName = "Reloading"),
	EQUIPPING			UMETA(DisplayName = "Equipping"),
	UNEQUIPPING			UMETA(DisplayName = "Unequipping")
};

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_BODY()

	//Item slot in Inventory 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GeneralConfig)
	int InventorySlotID;

	//Inventory GUI Image
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ItemConfig)
	UTexture2D* ThumbnailNormal;

	//Inventory GUI Image
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ItemConfig)
	UTexture2D* ThumbnailHover;

	//Name of the Item
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GeneralConfig)
	FText Name;

	//Check if its allowed to have multiple of same type in inventory
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GeneralConfig)
	bool Stackable;

	//Max amount of items in inventory
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GeneralConfig)
	int32 MaxStackSize;

	//Socket name on character hand to attach item when equiped.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GeneralConfig)
	FName AttachSocket;

	//Socket name on character body to attach item when unequipped.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GeneralConfig)
	FName DetachSocket;

	//Socket name on character body to attach item when unequipped.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GeneralConfig)
	FName MuzzleSocketName;

	//Time between shots when using Auto/Burst
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponConfig)
	float TimeBetweenShots;

	//Bool to check if Weapon have support for Rail-attachments
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponConfig)
	bool Picrail;

	//Remaining Ammo in Magazine
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MagazineConfig)
	int32 RemainingAmmo;

	//Magazine size
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MagazineConfig)
	int32 MagazineSize;
};

//Montage Struct used for Weapon related Pawn Animations 
USTRUCT()
struct FWeaponAnim
{
	GENERATED_BODY()

		/** animation played on Pawn */
		UPROPERTY(EditDefaultsOnly, Category = Animation)
		UAnimMontage* Pawn;
};

UCLASS()
class SHOOTERPROJECT_API AWeaponClass : public AActor
{
	GENERATED_BODY()


public:

	// PAWN ANIMATIONS
	/** equip animation */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim EquipAnim;

	/** unequip animation */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim UnequipAnim;

	/** fire animation */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim FireAnim;

	/** equip animation */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim ReloadAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Weapon)
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Weapon)
	TSubclassOf<AActor> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Weapon)
	FWeaponData WeaponData;

public:

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Fire();
	
public:	
	// Sets default values for this actor's properties
	AWeaponClass();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
