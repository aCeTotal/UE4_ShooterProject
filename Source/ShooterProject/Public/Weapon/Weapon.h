// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class UAnimMontage;
class AShooterProjectCharacter;
class UAudioComponent;
class UParticleSystemComponent;
class UForceFeedbackEffect;
class USoundCue;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Idle,
	Firing,
	Reloading,
	Equipping
};

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_BODY()

	//Clip Size
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo)
	int32 AmmoPerMagazine;

	//The item that this weapon uses as ammo
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo)
	TSubclassOf<class UMagazineItem> AmmoClass;

	//Time between two consecutive shots
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	float TimeBetweenShots;

	//Default values
	FWeaponData()
	{
		AmmoPerMagazine = 20;
		TimeBetweenShots = 0.2f;
	}	
};

USTRUCT()
struct FWeaponAnim
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* Pawn1P;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* Pawn3P;
};

USTRUCT(BlueprintType)
struct FHitScanConfiguration
{
	GENERATED_BODY()

	FHitScanConfiguration()
	{
		Distance = 10000;
		Damage = 25.f;
		Radius = 0.f;
		DamageType = UDamageType::StaticClass();
	}

	/**A map of bone -> damage amount. If the bone is a child of the given bone, it will use this damage amount.
	 * A value of 2 would mean double damage ect. */
	UPROPERTY(EditDefaultsOnly, Category = "Trace Info")
	TMap<FName, float> BoneDamageModifiers;

	//How far the hitscan traces for a hit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trace Info")
	float Distance;

	//The Amount of damage to deal when we hit a player with the hitscan
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trace Info")
	float Damage;

	//Optional trace radius. A value of zero is just a linetrace, anything higher is a spheretrace
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trace Info")
	float Radius;

	//Type of damage
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	TSubclassOf<UDamageType> DamageType;

};

UCLASS()
class SHOOTERPROJECT_API AWeapon : public AActor
{
	GENERATED_BODY()

	friend class AShooterProjectCharacter;
	
public:	
	// Sets default values for this actor's properties
	AWeapon();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Consume a bullet
	void UseMagazineAmmo();

	//Consume ammo from the inventory
	void ConsumeAmmo(const int32 Amount);

	//[server] return ammo to the inventory when the weapon is unequipped
	void ReturnAmmoToInventory();

	//Weapon is being equipped by owner pawn
	virtual void OnEquip();

	//Weapon is now equipped by owner pawn
	virtual void OnEquipFinished();

	//Weapon is holstered by owner pawn
	virtual void OnUnEquip();

	//check if it's currently equipped
	bool IsEquipped() const;

	//Check if mesh is already attached
	bool IsAttachedToPawn() const;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Input

	//[local + server] start weapon fire
	virtual void StartFire();

	//[local + server] stop weapon fire
	virtual void StopFire();

	//[all] start weapon reload
	virtual void StartReload(bool bFromReplication = false);

	//[local + server] interrupt weapon reload
	virtual void StopReload();

	//[server] performs actual reload
	virtual void ReloadWeapon();

	//trigger reload from server
	UFUNCTION(Reliable, Client)
	void ClientStartReload();

	bool CanFire() const;
	bool CanReload() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	EWeaponState GetCurrentState() const;

	//Get current ammo amount (total)
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const;

	//Get current ammo amount (Magazine)
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmoInMagazine() const;

	//Get Magazine size
	int32 GetAmmoPerMagazine() const;

	//Get weapon mesh (Needs pawn owner to determine variant)
	UFUNCTION(BlueprintPure, Category = "Weapon")
	USkeletalMeshComponent* GetWeaponMesh() const;

	//Get Pawn owner
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	class AShooterProjectCharacter* GetPawnOwner() const;

	//Set the weapon's owning pawn
	void SetPawnOwner(AShooterProjectCharacter* ShooterProjectCharacter);

	//get last time when this weapon was switches to
	float GetEquipStartedTime() const;

	//Gets the duration of equipping weapon
	float GetEquipDuration() const;

	//The weapon item in the players inventory
	UPROPERTY(Replicated, BlueprintReadOnly, Transient)
	class UWeaponItem* Item;

	//Pawn owner
	UPROPERTY(Transient, ReplicatedUsing = OnRep_PawnOwner)
	class AShooterProjectCharacter* PawnOwner;

	//Weapon data
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Config)
	FWeaponData WeaponConfig;

	//Line trace data. Will be used if projectiles class is null
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Config)
	FHitScanConfiguration HitScanConfig;
	

public:	

	//Weapon mesh
	UPROPERTY(EditAnywhere, Category = Components)
	USkeletalMeshComponent* WeaponMesh;

protected:

	//Adjust to handle frame rate affecting actual timer interval
	UPROPERTY(Transient)
	float TimerIntervalAdjustment;

	//Whether to allow automatic weapon to catch ut with shorter refire cycles
	UPROPERTY(Config)
	bool bAllowAutomaticWeaponCatchup = true;

	//firing audio (bLoopedFireSound)
	UPROPERTY(Transient)
	UAudioComponent* FireAC;

	//name of bone/socket for muzzle in weapon mesh
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName MuzzleAttachPoint;

	//name of socket to attach weapon mesh to the character when Equipped
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName AttachPoint;

	//name of socket to attach weapon mesh to the character when UnEquipped
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName DetachPoint;

	//FX for muzzle flash
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UParticleSystem* MuzzleFX;
	

};
