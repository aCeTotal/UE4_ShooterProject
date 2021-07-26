// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"
#include "ShooterProject/ShooterProject.h"

#include "Player/ShooterProjectPlayerController.h"
#include "Player/ShooterProjectCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Components/InventoryComponent.h"
#include "Curves/CurveVector.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Net/UnrealNetwork.h"
#include "Items/AmmoItem.h"
#include "Items/EquippableItem.h"
#include "DrawDebugHelpers.h"

// Sets default values
AWeapon::AWeapon()
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->bReceivesDecals = false;
	WeaponMesh->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	RootComponent = WeaponMesh;

	PrimarySight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrimarySight"));
	PrimarySight->bReceivesDecals = false;
	PrimarySight->SetCollisionObjectType(ECC_WorldDynamic);
	PrimarySight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrimarySight->SetCollisionResponseToAllChannels(ECR_Ignore);
	PrimarySight->SetupAttachment(WeaponMesh, FName("PrimarySightSocket"));

	SecondarySight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SecondarySight"));
	SecondarySight->bReceivesDecals = false;
	SecondarySight->SetCollisionObjectType(ECC_WorldDynamic);
	SecondarySight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SecondarySight->SetCollisionResponseToAllChannels(ECR_Ignore);
	SecondarySight->SetupAttachment(WeaponMesh, FName("SecondarySightSocket"));

	bLoopedMuzzleFX = false;
	bLoopedFireAnim = false;
	bPlayingFireAnim = false;
	bIsEquipped = false;
	bWantsToFire = false;
	bPendingReload = false;
	bPendingEquip = false;
	CurrentState = EWeaponState::Idle;
	AttachSocket = FName("GripPoint");
	MuzzleAttachPoint = FName("Muzzle");
	CurrentSight = PrimarySight;
	DistanceToSight = 25.f;
	SightIndex = 0;

	CurrentAmmoInWeapon = 0;
	BurstCounter = 0;
	LastFireTime = 0.0f;

	ADSTime = 1.f;
	RecoilResetSpeed = 5.f;
	RecoilSpeed = 10.f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bReplicates = true;
	bNetUseOwnerRelevancy = true;

}


void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, PawnOwner);

	DOREPLIFETIME_CONDITION(AWeapon, CurrentAmmoInWeapon, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeapon, BurstCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AWeapon, bPendingReload, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AWeapon, Item, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AWeapon, CurrentSight, COND_OwnerOnly);
}


void AWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	DetachMeshFromPawn();
}


// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	// Server sets PawnOwner at Spawn/GameStart
	if (HasAuthority())
	{
		PawnOwner = Cast<AShooterProjectCharacter>(GetOwner());
	}
}


//If Weapon gets destroyed, we stop WeaponFire aka Particles like Muzzle ect.
void AWeapon::Destroyed()
{
	Super::Destroyed();

	StopSimulatingWeaponFire();
}


//Function used when firing. Server reduces the amount of ammo in the weapon, each time the client fire a bullet.
void AWeapon::UseMagazineAmmo()
{
	if (HasAuthority())
	{
		--CurrentAmmoInWeapon;
	}
}


//When reloading the weapon, server consumes a magazine from the client Inventory
void AWeapon::ConsumeMagazine(const int32 Amount)
{
	if (HasAuthority() && PawnOwner)
	{
		//Get the player Inventory
		if (UInventoryComponent* Inventory = PawnOwner->PlayerInventory)
		{
			//Find Magazine inside the client inventory
			if (UItem* MagazineItem = Inventory->FindItemByClass(WeaponConfig.MagazineClass))
			{
				Inventory->ConsumeItem(MagazineItem, Amount);
			}
		}
	}
}


void AWeapon::ReturnMagazineToInventory(int32 OldAmmo)
{
	//When the weapon is unequipped or Reloaded before empty, try return the players ammo to their inventory
	if (HasAuthority())
	{
		if (PawnOwner)
		{
			if (UInventoryComponent* Inventory = PawnOwner->PlayerInventory)
			{
				Inventory->TryAddMagazineFromClass(WeaponConfig.MagazineClass, 1, OldAmmo);
			}
		}
	}
}


void AWeapon::OnEquip()
{
	AttachMeshToPawn();
	
	bPendingEquip = true;
	DetermineWeaponState();

	OnEquipFinished();

	if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		PlayWeaponSound(EquipSound);
	}
}


void AWeapon::OnEquipFinished()
{
	AttachMeshToPawn();

	bIsEquipped = true;
	bPendingEquip = false;

	// Determine the state so that the "Can Reload" checks work
	DetermineWeaponState();
}


void AWeapon::OnUnEquip()
{
	DetachMeshFromPawn();
	bIsEquipped = false;
	StopFire();

	if (bPendingReload)
	{
		StopWeaponAnimation(ReloadAnim);
		bPendingReload = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_StopReload);
		GetWorldTimerManager().ClearTimer(TimerHandle_ReloadWeapon);
	}

	if (bPendingEquip)
	{
		StopWeaponAnimation(EquipAnim);
		bPendingEquip = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_OnEquipFinished);
	}

	//ReturnMagazineToInventory();
	DetermineWeaponState();
}


bool AWeapon::IsEquipped() const
{
	return bIsEquipped;
}


bool AWeapon::IsAttachedToPawn() const
{
	return bIsEquipped || bPendingEquip;
}


void AWeapon::StartFire()
{
	if (!HasAuthority())
	{
		ServerStartFire();
	}

	if (!bWantsToFire)
	{
		bWantsToFire = true;
		DetermineWeaponState();
	}
}


void AWeapon::StopFire()
{
	if (!HasAuthority() && PawnOwner && PawnOwner->IsLocallyControlled())
	{
		ServerStopFire();
	}

	if (bWantsToFire)
	{
		bWantsToFire = false;
		DetermineWeaponState();
	}
}


void AWeapon::StartReload(bool bFromReplication)
{
	if (!bFromReplication && !HasAuthority())
	{
		ServerStartReload();
	}

	if (bFromReplication || CanReload())
	{
		bPendingReload = true;
		DetermineWeaponState();

		float AnimDuration = PlayWeaponAnimation(ReloadAnim);
		if (AnimDuration <= 0.0f)
		{
			AnimDuration = .5f;
		}

		GetWorldTimerManager().SetTimer(TimerHandle_StopReload, this, &AWeapon::StopReload, AnimDuration, false);
		if (HasAuthority())
		{
			GetWorldTimerManager().SetTimer(TimerHandle_ReloadWeapon, this, &AWeapon::ReloadWeapon, FMath::Max(0.1f, AnimDuration - 0.1f), false);
		}

		if (PawnOwner && PawnOwner->IsLocallyControlled())
		{
			PlayWeaponSound(ReloadSound);
		}
	}
}


void AWeapon::StopReload()
{
	if (CurrentState == EWeaponState::Reloading)
	{
		bPendingReload = false;
		DetermineWeaponState();
		StopWeaponAnimation(ReloadAnim);
	}
}


void AWeapon::ReloadWeapon()
{
	int32 NewMagazineAmmo = GetNewAmmo();

	//Returning non-empty magazines to inventory does almost work, but it has some wierd behavior.
	/**if (CurrentAmmoInWeapon > 0)
	{
		ReturnMagazineToInventory(CurrentAmmoInWeapon);
	}*/
	
	if (NewMagazineAmmo > 0)
	{
		CurrentAmmoInWeapon = 0;
		CurrentAmmoInWeapon += NewMagazineAmmo;
		ConsumeMagazine(NewMagazineAmmo);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Didnt have enough ammo for a reload"));		
	}
}


bool AWeapon::CanFire() const
{
	bool bCanFire = PawnOwner != nullptr;
	bool bStateOKToFire = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));
	return ((bCanFire == true) && (bStateOKToFire == true) && (bPendingReload == false));
}


bool AWeapon::CanReload() const
{
	bool bCanReload = PawnOwner != nullptr;
	bool bGotAmmo = (CurrentAmmoInWeapon < WeaponConfig.MaxAmmo) && (GetNewAmmo() > 0);
	bool bStateOKToReload = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));
	return ((bCanReload == true) && (bGotAmmo == true) && (bStateOKToReload == true));
}


EWeaponState AWeapon::GetCurrentState() const
{
	return CurrentState;
}

int32 AWeapon::GetNewAmmo() const
{
	if (PawnOwner)
	{
		if (UInventoryComponent* Inventory = PawnOwner->PlayerInventory)
		{
			if (UItem* Ammo = Inventory->FindItemByClass(WeaponConfig.MagazineClass))
			{
				return Ammo->GetAmmo();
			}
		}
	}

	return 0;
}


int32 AWeapon::GetCurrentAmmoInWeapon() const
{
	return CurrentAmmoInWeapon;
}


int32 AWeapon::GetAmmoPerMagazine() const
{
	return WeaponConfig.MaxAmmo;
}


USkeletalMeshComponent* AWeapon::GetWeaponMesh() const
{
	return WeaponMesh;
}


class AShooterProjectCharacter* AWeapon::GetPawnOwner() const
{
	return PawnOwner;
}


void AWeapon::SetPawnOwner(AShooterProjectCharacter* Character)
{
	if (PawnOwner != Character)
	{
		SetInstigator(Character);
		PawnOwner = Character;
		// net owner for RPC calls
		SetOwner(Character);
	}
}


float AWeapon::GetEquipStartedTime() const
{
	return EquipStartedTime;
}


float AWeapon::GetEquipDuration() const
{
	return EquipDuration;
}


void AWeapon::ClientStartReload_Implementation()
{
	StartReload();
}


void AWeapon::ServerStartFire_Implementation()
{
	StartFire();
}


bool AWeapon::ServerStartFire_Validate()
{
	return true;
}


void AWeapon::ServerStopFire_Implementation()
{
	StopFire();
}


bool AWeapon::ServerStopFire_Validate()
{
	return true;
}


void AWeapon::ServerStartReload_Implementation()
{
	StartReload();
}


bool AWeapon::ServerStartReload_Validate()
{
	return true;
}


void AWeapon::ServerStopReload_Implementation()
{
	StopReload();
}


bool AWeapon::ServerStopReload_Validate()
{
	return true;
}


void AWeapon::OnRep_PawnOwner()
{

}


void AWeapon::OnRep_BurstCounter()
{
	if (BurstCounter > 0)
	{
		SimulateWeaponFire();
	}
	else
	{
		StopSimulatingWeaponFire();
	}
}


void AWeapon::OnRep_Reload()
{
	if (bPendingReload)
	{
		StartReload();
	}
	else
	{
		StopReload();
	}
}


void AWeapon::SimulateWeaponFire()
{
	if (HasAuthority() && CurrentState != EWeaponState::Firing)
	{
		return;
	}

	if (MuzzleFX)
	{
		if (!bLoopedMuzzleFX || MuzzlePSC == NULL)
		{
			// Split screen requires we create 2 effects. One that we see and one that the other player sees.
			if ((PawnOwner != NULL) && (PawnOwner->IsLocallyControlled() == true))
			{
				AController* PlayerCon = PawnOwner->GetController();
				if (PlayerCon != NULL)
				{
					WeaponMesh->GetSocketLocation(MuzzleAttachPoint);
					MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, WeaponMesh, MuzzleAttachPoint);
					MuzzlePSC->bOwnerNoSee = false;
					MuzzlePSC->bOnlyOwnerSee = true;
				}
			}
			else
			{
				MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, WeaponMesh, MuzzleAttachPoint);
			}
		}
	}


	if (!bLoopedFireAnim || !bPlayingFireAnim)
	{
		FWeaponAnim AnimToPlay = FireAnim; //PawnOwner->IsAiming() || PawnOwner->IsLocallyControlled() ? FireAimingAnim : FireAnim;
		PlayWeaponAnimation(FireAnim);
		bPlayingFireAnim = true;
	}

	if (bLoopedFireSound)
	{
		if (FireAC == NULL)
		{
			FireAC = PlayWeaponSound(FireLoopSound);
		}
	}
	else
	{
		PlayWeaponSound(FireSound);
	}

	AShooterProjectPlayerController* PC = (PawnOwner != NULL) ? Cast<AShooterProjectPlayerController>(PawnOwner->Controller) : NULL;
	if (PC != NULL && PC->IsLocalController())
	{
		if (FireCameraShake != NULL)
		{
			PC->ClientStartCameraShake(FireCameraShake, 2);
		}
		if (FireForceFeedback != NULL)
		{
			FForceFeedbackParameters FFParams;
			FFParams.Tag = "Weapon";
			PC->ClientPlayForceFeedback(FireForceFeedback, FFParams);
		}
	}
}


void AWeapon::StopSimulatingWeaponFire()
{
	if (bLoopedMuzzleFX)
	{
		if (MuzzlePSC != NULL)
		{
			MuzzlePSC->DeactivateSystem();
			MuzzlePSC = NULL;
		}
		if (MuzzlePSCSecondary != NULL)
		{
			MuzzlePSCSecondary->DeactivateSystem();
			MuzzlePSCSecondary = NULL;
		}
	}

	if (bLoopedFireAnim && bPlayingFireAnim)
	{
		StopWeaponAnimation(FireAimingAnim);
		StopWeaponAnimation(FireAnim);
		bPlayingFireAnim = false;
	}

	if (FireAC)
	{
		FireAC->FadeOut(0.1f, 0.0f);
		FireAC = NULL;

		PlayWeaponSound(FireFinishSound);
	}
}


void AWeapon::HandleHit(const FHitResult& Hit, class AShooterProjectCharacter* HitPlayer /*= nullptr*/)
{
	if (Hit.GetActor())
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit actor %s"), *Hit.GetActor()->GetName());
	}

	ServerHandleHit(Hit, HitPlayer);

	if (HitPlayer && PawnOwner)
	{
		if (AShooterProjectPlayerController* PC = Cast<AShooterProjectPlayerController>(PawnOwner->GetController()))
		{
			PC->OnHitPlayer();
		}
	}
}


void AWeapon::ServerHandleHit_Implementation(const FHitResult& Hit, class AShooterProjectCharacter* HitPlayer /*= nullptr*/)
{
	if (PawnOwner)
	{
		float DamageMultiplier = 1.f;

		/**Certain bones like head might give extra damage if hit. Apply those.*/
		for (auto& BoneDamageModifier : HitScanConfig.BoneDamageModifiers)
		{
			if (Hit.BoneName == BoneDamageModifier.Key)
			{
				DamageMultiplier = BoneDamageModifier.Value;
				break;
			}
		}

		if (HitPlayer)
		{
			UGameplayStatics::ApplyPointDamage(HitPlayer, HitScanConfig.Damage * DamageMultiplier, (Hit.TraceStart - Hit.TraceEnd).GetSafeNormal(), Hit, PawnOwner->GetController(), this, HitScanConfig.DamageType);
		}
	}
}

bool AWeapon::ServerHandleHit_Validate(const FHitResult& Hit, class AShooterProjectCharacter* HitPlayer /*= nullptr*/)
{
	return true;
}


void AWeapon::FireShot()
{
	if (PawnOwner)
	{
		if (AShooterProjectPlayerController* PC = Cast<AShooterProjectPlayerController>(PawnOwner->GetController()))
		{
			if (RecoilCurve)
			{
				const FVector2D RecoilAmount(RecoilCurve->GetVectorValue(FMath::RandRange(0.f, 1.f)).X, RecoilCurve->GetVectorValue(FMath::RandRange(0.f, 1.f)).Y);
				PC->ApplyRecoil(RecoilAmount, RecoilSpeed, RecoilResetSpeed, FireCameraShake);
			}

			FVector SocketLoc;
			FRotator SocketRot;
			SocketLoc = GetWeaponMesh()->GetSocketLocation("MuzzleAttachPoint");
			SocketRot = GetWeaponMesh()->GetSocketRotation("MuzzleAttachPoint");
			
			FHitResult Hit;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);
			QueryParams.AddIgnoredActor(PawnOwner);

			FVector FireDir = SocketRot.Vector();
			FVector TraceStart = SocketLoc;
			FVector TraceEnd = (FireDir * HitScanConfig.Distance) + SocketLoc;

			if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, COLLISION_WEAPON, QueryParams))
			{
				AShooterProjectCharacter* HitChar = Cast<AShooterProjectCharacter>(Hit.GetActor());

				HandleHit(Hit, HitChar);

				FColor PointColor = FColor::Red;
				DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 5.f, PointColor, false, 30.f);
			}

		}
	}
}


void AWeapon::HandleReFiring()
{
	UWorld* MyWorld = GetWorld();

	float SlackTimeThisFrame = FMath::Max(0.0f, (MyWorld->TimeSeconds - LastFireTime) - WeaponConfig.TimeBetweenShots);

	if (bAllowAutomaticWeaponCatchup)
	{
		TimerIntervalAdjustment -= SlackTimeThisFrame;
	}

	HandleFiring();
}


void AWeapon::HandleFiring()
{

	if ((CurrentAmmoInWeapon > 0) && CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (PawnOwner && PawnOwner->IsLocallyControlled())
		{
			FireShot();
			UseMagazineAmmo();

			// update firing FX on remote clients if function was called on server
			BurstCounter++;
		}
	}
	else if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		if (GetNewAmmo() == 0 && !bRefiring)
		{
			PlayWeaponSound(OutOfAmmoSound);
			AShooterProjectPlayerController* MyPC = Cast<AShooterProjectPlayerController>(PawnOwner->Controller);
		}

		// stop weapon fire FX, but stay in Firing state
		if (BurstCounter > 0)
		{
			OnBurstFinished();
		}
	}

	if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		// local client will notify server
		if (GetLocalRole())
		{
			ServerHandleFiring();
		}

		// setup refire timer
		bRefiring = (CurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AWeapon::HandleReFiring, FMath::Max<float>(WeaponConfig.TimeBetweenShots + TimerIntervalAdjustment, SMALL_NUMBER), false);
			TimerIntervalAdjustment = 0.f;
		}
	}

	LastFireTime = GetWorld()->GetTimeSeconds();
}


void AWeapon::OnBurstStarted()
{
	// start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
		LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AWeapon::HandleFiring, LastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
	}
	else
	{
		HandleFiring();
	}
}


void AWeapon::OnBurstFinished()
{
	// stop firing FX on remote clients
	BurstCounter = 0;

	// stop firing FX locally, unless it's a dedicated server
	if (GetNetMode() != NM_DedicatedServer)
	{
		StopSimulatingWeaponFire();
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_HandleFiring);
	bRefiring = false;

	// reset firing interval adjustment
	TimerIntervalAdjustment = 0.0f;
}


void AWeapon::SetWeaponState(EWeaponState NewState)
{
	const EWeaponState PrevState = CurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}
}


void AWeapon::DetermineWeaponState()
{
	EWeaponState NewState = EWeaponState::Idle;

	if (bIsEquipped)
	{
		if (bPendingReload)
		{
			if (CanReload() == false)
			{
				NewState = CurrentState;
			}
			else
			{
				NewState = EWeaponState::Reloading;
			}
		}
		else if ((bPendingReload == false) && (bWantsToFire == true) && (CanFire() == true))
		{
			NewState = EWeaponState::Firing;
		}
	}
	else if (bPendingEquip)
	{
		NewState = EWeaponState::Equipping;
	}

	SetWeaponState(NewState);
}


void AWeapon::AttachMeshToPawn()
{
	if (PawnOwner)
	{
		// Remove and hide both first and third person meshes
		DetachMeshFromPawn();

		USkeletalMeshComponent* PawnMesh = PawnOwner->Get1PMesh();
		AttachToComponent(PawnOwner->Get1PMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocket);
	}
}


void AWeapon::DetachMeshFromPawn()
{

}


UAudioComponent* AWeapon::PlayWeaponSound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	if (Sound && PawnOwner)
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, PawnOwner->GetRootComponent());
	}

	return AC;
}


float AWeapon::PlayWeaponAnimation(const FWeaponAnim& Animation)
{
	float Duration = 0.0f;
	if (PawnOwner)
	{
		UAnimMontage* UseAnim = PawnOwner->IsLocallyControlled() ? Animation.Pawn1P : Animation.Pawn3P;
		if (UseAnim)
		{
			Duration = PawnOwner->PlayAnimMontage(UseAnim);
		}
	}

	return Duration;
}


void AWeapon::StopWeaponAnimation(const FWeaponAnim& Animation)
{
	if (PawnOwner)
	{
		UAnimMontage* UseAnim = PawnOwner->IsLocallyControlled() ? Animation.Pawn1P : Animation.Pawn3P;
		if (UseAnim)
		{
			PawnOwner->StopAnimMontage(UseAnim);
		}
	}
}


FVector AWeapon::GetCameraAim() const
{
	AShooterProjectPlayerController* const PlayerController = GetInstigator() ? Cast<AShooterProjectPlayerController>(GetInstigator()->Controller) : NULL;
	FVector FinalAim = FVector::ZeroVector;

	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (GetInstigator())
	{
		FinalAim = GetInstigator()->GetBaseAimRotation().Vector();
	}

	return FinalAim;
}


FHitResult AWeapon::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const
{
	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(WeaponTrace), true, GetInstigator());
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, COLLISION_WEAPON, TraceParams);

	return Hit;
}


void AWeapon::ServerHandleFiring_Implementation()
{
	const bool bShouldUpdateAmmo = (CurrentAmmoInWeapon > 0 && CanFire());

	HandleFiring();

	if (bShouldUpdateAmmo)
	{
		// update ammo
		UseMagazineAmmo();

		// update firing FX on remote clients
		BurstCounter++;
	}
}

bool AWeapon::ServerHandleFiring_Validate()
{
	return true;
}
