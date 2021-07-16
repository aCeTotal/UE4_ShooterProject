// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Items/EquippableItem.h"
#include "Components/BoxComponent.h"
#include "ShooterProjectCharacter.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquippedItemsChanged, const EEquippableSlot, Slot, const UEquippableItem*, Item);

class AWeaponClass;

//AnimBP States.
UENUM(BlueprintType)
enum class EPawnState : uint8
{
	None			UMETA(DisplayName = "None"),
	Stand			UMETA(DisplayName = "Stance_Stand"),
	Crouch			UMETA(DisplayName = "Stance_Crouch"),
	Prone			UMETA(DisplayName = "Stance_Prone")
};

/**Upper-body states while using a weapon. Cached in AnimInstance as bools.
 * This will set the correct upper-body base state and select the correct AimOffset.
 */
UENUM(BlueprintType)
enum class EWeaponOffsetState : uint8
{
	Resting			UMETA(DisplayName = "Weapon_Resting"),
	Ready			UMETA(DisplayName = "Weapon_Ready"),
	Aiming			UMETA(DisplayName = "Weapon_Aiming")
};

USTRUCT()
struct FInteractionData
{
	GENERATED_BODY()

		//Default values
		FInteractionData()
	{
		ViewedInteractionComponent = nullptr;
		LastInteractionCheckTime = 0.f;
		bInteractHeld = false;
	}

	//The current interactable component we're viewing, if there is one
	UPROPERTY()
	class UInteractionComponent* ViewedInteractionComponent;

	//The time when we last checked for an interactable
	UPROPERTY()
	float LastInteractionCheckTime;

	//Wether the local player is holding the interact key
	UPROPERTY()
	bool bInteractHeld;
};

UCLASS(config=Game)
class AShooterProjectCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* LagBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

public:
	AShooterProjectCharacter();

	//The Mesh to have equipped if we don't have an item equipped - ie. the bare skin meshes.
	UPROPERTY(BlueprintReadOnly, Category = Mesh)
	TMap<EEquippableSlot, USkeletalMesh*> NakedMeshes;

	//The players body meshes
	UPROPERTY(BlueprintReadOnly, Category = Mesh)
	TMap<EEquippableSlot, USkeletalMeshComponent*> PlayerMeshes;

	/**Modular character */
	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* HelmetMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* ChestMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* LegsMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* FeetMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* VestMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* HandsMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* BackpackMesh;

	UFUNCTION(BlueprintCallable)
	void SetLootSource(class UInventoryComponent* NewLootSource);

	UFUNCTION(BlueprintPure, Category = "Looting")
	bool IsLooting() const;

	UFUNCTION()
	bool IsHoldingWeapon() const;

	UFUNCTION()
	void ResetProneFix();


protected:

	//Begin being looted by a player
	UFUNCTION()
	void BeginLootingPlayer(class AShooterProjectCharacter* Character);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void ServerSetLootSource(class UInventoryComponent* NewLootSource);

	//The inventory we are currently looting from
	UPROPERTY(ReplicatedUsing = OnRep_LootSource, BlueprintReadOnly)
	UInventoryComponent* LootSource;

	UFUNCTION()
	void OnLootSourceOwnerDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void OnRep_LootSource();

public:

	UFUNCTION(BlueprintCallable, Category = "Looting")
	void LootItem(class UItem* ItemToGive);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerLootItem(class UItem* ItemToLoot);

	/**Our player inventory */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent* PlayerInventory;

	/**Interaction component used to allow other players to loot us when we have died*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UInteractionComponent* LootPlayerInteraction;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	//The current stance of the player character
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Animation, Replicated)
	EPawnState CurrentPawnState;

	//The current stance of the player character
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Animation, Replicated)
	EPawnState ProneFix;

	//The current state of the upper-body while holding a weapon.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Animation, Replicated)
	EWeaponOffsetState CurrentOffsetState;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUpdatePawnState(EPawnState NewState, EPawnState ProneCheckValue);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUpdateOffsetState(EWeaponOffsetState NewState);

	// Indicates whether the Player Character is running or not.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsRunning = false;

	// Indicates whether the Player Character is sprinting or not.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsSprinting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float Runningspeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float Sprintingspeed;

	//Time before resetting the ProneFix-value (Animation)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float ProneToStandDelay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float ProneToCrouchDelay;

protected:

	//How often in seconds to check for an interactable object. Set this to zero if you want to check every tick.
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionCheckFrequency;

	//How far we'll trace when we check if the player is looking at an interactable object
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionCheckDistance;

	void PerformInteractionCheck();

	void CouldntFindInteractable();
	void FoundNewInteractable(UInteractionComponent* Interactable);

	void BeginInteract();
	void EndInteract();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerBeginInteract();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEndInteract();

	void Interact();

	//Information about the current state of the players interaction
	UPROPERTY()
	FInteractionData InteractionData;

	//Helper function to make grabbing interactable faster
	FORCEINLINE class UInteractionComponent* GetInteractable() const { return InteractionData.ViewedInteractionComponent; }

	FTimerHandle TimerHandle_Interact;

	/* Handles to manage Pronefix bool, so that aimoffsets are deactivated when standing up from prone state */
	FTimerHandle ProneToCrouchTimerHandle;
	FTimerHandle ProneToStandTimerHandle;

public:
	
	//True if we're interactive with an item that has an interaction time (For example an lamp that takes 2 seconds to turn on)
	bool IsInteracting() const;

	//Get the time till we interact with the current interactable
	float GetRemainingInteractTime() const;

	// Items

	/**[Server] Use an item from our inventory*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void UseItem(class UItem* Item);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUseItem(class UItem* Item);

	/**[Server] Drop an item from our inventory*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void DropItem(class UItem* Item, const int32 Quantity);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerDropItem(class UItem* Item, const int32 Quantity);

	/**We need this because the pickups use a blueprint base class*/
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	TSubclassOf<class APickup> PickupClass;

public:

	bool EquipItem(class UEquippableItem* Item);
	bool UnEquipItem(class UEquippableItem* Item);

	void EquipGear(class UGearItem* Gear);
	void UnEquipGear(const EEquippableSlot Slot);

	void EquipWeapon(class UWeaponItem* WeaponItem);
	void UnEquipWeapon();


	UPROPERTY(BlueprintAssignable, Category = "Items")
	FOnEquippedItemsChanged OnEquippedItemsChanged;

	UFUNCTION(BlueprintPure)
	class USkeletalMeshComponent* GetSlotSkeletalMeshComponent(const EEquippableSlot Slot);

	UFUNCTION(BlueprintPure)
	FORCEINLINE TMap<EEquippableSlot, UEquippableItem*> GetEquippedItems() const { return EquippedItems; };

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	FORCEINLINE class AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }


protected:

	//Allows for efficient access of equipped item
	UPROPERTY(VisibleAnywhere, Category = "Items")
	TMap<EEquippableSlot, UEquippableItem*> EquippedItems;

	void StartCrouching();
	void StopCrouching();

	void StartProning();
	void StopProning();

	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Health")
	float Health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float MaxHealth;

public:

	//Modify the players health by either a negative or positive amount. Return the amount of health actually removed.
	float ModifyHealth(const float Delta);

	UFUNCTION()
	void OnRep_Health(float OldHealth);

	UFUNCTION(BlueprintImplementableEvent)
	void OnHealthModified(const float HealthDelta);

	void StartReload();

	



protected:

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_EquippedWeapon)
	class AWeapon* EquippedWeapon;

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void StartFire();
	void StopFire();

	void BeginMeleeAttach();

	UFUNCTION(Server, Reliable)
	void ServerProcessMeleeHit(const FHitResult& MeleeHit);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayMeleeFX();

	//MeleeAttach
	UPROPERTY()
	float LastMeleeAttackTime;
	
	UPROPERTY(EditDefaultsOnly, Category = Melee)
	float MeleeAttachDistance;

	UPROPERTY(EditDefaultsOnly, Category = Melee)
	float MeleeAttachDamage;

	UPROPERTY(EditDefaultsOnly, Category = Melee)
	class UAnimMontage* MeleeAttackMontage;


	//Called when killed by a player, or killed by something else like the environment
	void Suicide(struct FDamageEvent const& DamageEvent, const AActor* DamageCauser);
	void KilledByPlayer(struct FDamageEvent const& DamageEvent, const class AShooterProjectPlayerController* EventInstigator, const AActor* DamageCauser);

	UPROPERTY(ReplicatedUsing = OnRep_Killer)
	class AShooterProjectCharacter* Killer;

	UFUNCTION()
	void OnRep_Killer();

	UFUNCTION(BlueprintImplementableEvent)
	void OnDeath();


	// Function used for selecting the next pawn stance in AnimBP.
	void NextPawnState();

	//Function used for selecting the prev pawn stance in AnimBP.
	void PrevPawnState();

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Restart() override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// Aiming

	bool CanAim() const;

	void StartAiming();
	void StopAiming();

	void SetAiming(const bool bNewAiming, EWeaponOffsetState NewState);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetAiming(const bool bNewAiming, EWeaponOffsetState NewState);

	UPROPERTY(Transient, Replicated)
	bool bIsAiming;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsAlive() const { return Killer == nullptr; };
	
	UFUNCTION(BlueprintPure, Category = "Weapons")
	FORCEINLINE bool IsAiming() const { return bIsAiming;}
};

