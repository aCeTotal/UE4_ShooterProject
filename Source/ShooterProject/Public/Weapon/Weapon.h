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
class UCurveVector;

class UPlayerAnimInstance;

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

	//Magazine Size
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo)
	int32 MaxAmmo;

	//The item that this weapon uses as ammo
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo)
	TSubclassOf<class UAmmoItem> MagazineClass;

	//Time between two consecutive shots
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	float TimeBetweenShots;

	//Default values
	FWeaponData()
	{
		MaxAmmo = 20;
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

	UPROPERTY()
	UPlayerAnimInstance* PlayerAnimInstance;

	// Consume a bullet
	void UseMagazineAmmo();

	//Consume Magazine from the inventory
	void ConsumeMagazine(const int32 Amount);

	//[server] return ammo to the inventory when the weapon is unequipped
	void ReturnMagazineToInventory(int32 OldAmmo);

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

	//Kan være feil
	UPROPERTY(BlueprintReadOnly, Category = Weapon)
	EWeaponState CurrentState;

	//Get current ammo amount (total)
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetNewAmmo() const;

	//Get current ammo amount (Magazine)
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmoInWeapon() const;

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

	UPROPERTY(EditAnywhere, Category = "Components")
	UStaticMeshComponent* PrimarySight;

	UPROPERTY(EditAnywhere, Category = "Components")
	UStaticMeshComponent* WeaponHipLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Components")
	UStaticMeshComponent* CurrentSight;

	UPROPERTY(EditAnywhere, Category = "Components")
	float DistanceToSight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curves")
	UCurveVector* Aiming_IdleCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curves")
	UCurveVector* Aiming_SlowWalkCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curves")
	UCurveVector* Aiming_FastWalkCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curves")
	UCurveVector* Hip_IdleCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curves")
	UCurveVector* Hip_SlowWalkCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curves")
	UCurveVector* Hip_FastWalkCurve;

	//Recoil
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float InterpFinalRecoil_Speed;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float InterpRecoil_Speed;

	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilLocation_X_Min;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilLocation_X_Max;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilLocation_Y_Min;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilLocation_Y_Max;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilLocation_Z_Min;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilLocation_Z_Max;

	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilRotation_Pitch_Min;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilRotation_Pitch_Max;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilRotation_Yaw_Min;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilRotation_Yaw_Max;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilRotation_Roll_Min;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilRotation_Roll_Max;

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
	FName AttachSocket;

	//name of socket to attach weapon mesh to the character when UnEquipped
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName DetachPoint;

	//FX for muzzle flash
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UParticleSystem* MuzzleFX;

	//Spawned component for muzzle FX
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzlePSC;

	//Spawned component for secondary muzzle FX
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzlePSCSecondary;

	//Camera shake on firing
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	TSubclassOf<UMatineeCameraShake> FireCameraShake;
	
	//The time it takes to aim down sights, in seconds
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	float ADSTime;

	//The amount of record to apply. We shoose a random point from 0-1 on the curve and use it to drive recoil.
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	class UCurveVector* RecoilCurve;

	//The speed at witch the recoil bumbs up per second
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilSpeed;

	//The speed at which the recoil resets per seconds
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilResetSpeed;

	//Force feedback effect to play when the weapon is fired
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UForceFeedbackEffect *FireForceFeedback;


	
	//////////////////////////////////////////////////////////////////////////////////////////
	// Animation and sound
	
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* ReloadTestAnim;
	
	//Single fire sound (bLoopedFIreSound not set)
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* FireSound;

	//Looped fire sound (bLoopedFireSound set)
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* FireLoopSound;

	//Finished burst sound (bLoopedFireSound set)
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* FireFinishSound;

	//Out of ammo sound
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* OutOfAmmoSound;

	//Reload sound
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* ReloadSound;

	//Reload animations
	//UPROPERTY(EditDefaultsOnly, Category = Animation)
	//FWeaponAnim ReloadAnim;

	//Equip sound
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* EquipSound;

	//Equip animations
	//UPROPERTY(EditDefaultsOnly, Category = Animation)
	//FWeaponAnim EquipAnim;

	//Fire animations
	//UPROPERTY(EditDefaultsOnly, Category = Animation)
	//FWeaponAnim FireAnim;

	//Fire animations
	//UPROPERTY(EditDefaultsOnly, Category = Animation)
	//FWeaponAnim FireAimingAnim;



	
	//////////////////////////////////////////////////////////////////////////////////////////
	// Flags

	//Is muzzle FX looped?
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	uint32 bLoopedMuzzleFX : 1;

	//Is fire sound looped?
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	uint32 bLoopedFireSound : 1;

	//Is fire animation looped?
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	uint32 bLoopedFireAnim : 1;

	//Is fire animation playing?
	uint32 bPlayingFireAnim : 1;

	//Is weapon currently equipped?
	uint32 bIsEquipped : 1;

	//Is weapon fire active?
	uint32 bWantsToFire : 1;

	//Is reload animation playing?
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Reload)
	uint32 bPendingReload : 1;

	//is equip animation playing?
	uint32 bPendingEquip : 1;

	//Weapon is refiring
	uint32 bRefiring;

	//////////////////////////////////////////////////////////////////////////////////////////



	
	//time of loast successful weapon fire
	float LastFireTime;

	//Last time when this weapon was switched to
	float EquipStartedTime;

	//How much time weapon needs to be equipped
	float EquipDuration;

	//current ammo - inside magazine
	UPROPERTY(EditAnywhere, Transient, Replicated)
	int32 CurrentAmmoInWeapon;

	//current ammo - inside magazine
	UPROPERTY(EditAnywhere, Transient, Replicated)
	int32 OldMagazineAmmoAmount;

	//burst counter, used for replicating fire events to remote clients
	UPROPERTY(Transient, ReplicatedUsing = OnRep_BurstCounter)
	int32 BurstCounter;



	
	//////////////////////////////////////////////////////////////////////////////////////////
	// TimerHandles

	//Handle for efficient management of OnEquipFinished timer
	FTimerHandle TimerHandle_OnEquipFinished;

	//Handle for efficient management of StopReload timer
	FTimerHandle TimerHandle_StopReload;

	//Handle for efficient management of ReloadWeapon timer
	FTimerHandle TimerHandle_ReloadWeapon;

	//Handle for efficient management of HandleFiring timer
	FTimerHandle TimerHandle_HandleFiring;



	
	//////////////////////////////////////////////////////////////////////////////////////////
	// Input - Server side

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerStartFire();

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerStopFire();

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerStartReload();

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerStopReload();



	
	//////////////////////////////////////////////////////////////////////////////////////////
	// Replication & Effects

	UFUNCTION()
	void OnRep_PawnOwner();

	UFUNCTION()
	void OnRep_BurstCounter();

	UFUNCTION()
	void OnRep_Reload();

	//Called in network play to do the cosmetic FX for firing
	virtual void SimulateWeaponFire();

	//Called in network play to stop cosmetic FX (e.g. for a looping shot)
	virtual void StopSimulatingWeaponFire();


	
	//////////////////////////////////////////////////////////////////////////////////////////
	// Weapon usage

	//Handle hit locally before asking server to process hit
	void HandleHit(const FHitResult& Hit, class AShooterProjectCharacter* HitPlayer = nullptr);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerHandleHit(const FHitResult& Hit, class AShooterProjectCharacter* HitPlayer = nullptr);

	//[local] weapon specific fire implementation
	virtual void FireShot();

	//[server] fire & update ammo
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerHandleFiring();

	//[local + server] handle weapon refire, compensating for slack time of the timer can't sample fast enough
	void HandleReFiring();
	
	//[local + server] Handle weapon fire
	void HandleFiring();

	//[local + server] firing started
	virtual void OnBurstStarted();

	//[local + server] firing finished
	virtual void OnBurstFinished();

	//Update weapon state
	void SetWeaponState(EWeaponState NewState);

	//Determine current weapon state
	void DetermineWeaponState();

	//Attaches weapon mesh to pawn's mesh
	void AttachMeshToPawn();

	//detaches weapon mesh from pawn
	void DetachMeshFromPawn();
	
	//////////////////////////////////////////////////////////////////////////////////////////
	// ANIMATIONS

	UPROPERTY(EditDefaultsOnly, Category = "1PAnimations")
	class UAnimMontage* Reload;

	UPROPERTY(EditDefaultsOnly, Category = "1PAnimations")
	class UAnimMontage* Fire;

	UPROPERTY(EditDefaultsOnly, Category = "1PAnimations")
	class UAnimMontage* FireAiming;

	UPROPERTY(EditDefaultsOnly, Category = "1PAnimations")
	class UAnimMontage* Equip;

	UPROPERTY(EditDefaultsOnly, Category = "1PAnimations")
	class UAnimMontage* UnEquip;
	
	//////////////////////////////////////////////////////////////////////////////////////////
	// Weapon usage helpers

	//Play weapon sounds
	UAudioComponent* PlayWeaponSound(USoundCue* Sound);

	//Play Weapon animations on arms
	float Play1PWeaponAnimation(UAnimMontage* Animation);

	//Stop playing weapon animation on arms
	void Stop1PWeaponAnimation(UAnimMontage* Animation);

	//Get the aim of the camera
	FVector GetCameraAim() const;

	//Find hit
	FHitResult WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const;

public:

	/** Returns FollowCamera subobject **/
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UStaticMeshComponent* GetCurrentSight() const { return CurrentSight; }

	/** Returns FollowCamera subobject **/
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UStaticMeshComponent* GetHipLocation() const { return WeaponHipLocation; }
	
};
