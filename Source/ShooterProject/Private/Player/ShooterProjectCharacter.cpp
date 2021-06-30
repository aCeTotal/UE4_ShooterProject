// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/ShooterProjectCharacter.h"

#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/InteractionComponent.h"
#include "Components/InventoryComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Items/EquippableItem.h"
#include "Items/GearItem.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstance.h"
#include "Net/UnrealNetwork.h"
#include "Net/RepLayout.h"
#include "Particles/Collision/ParticleModuleCollisionGPU.h"
#include "Player/ShooterProjectPlayerController.h"
#include "ShooterProject/ShooterProject.h"
#include "World/Pickup.h"

#define LOCTEXT_NAMESPACE "ShooterProjectCharacter"

//////////////////////////////////////////////////////////////////////////
// AShooterProjectCharacter

AShooterProjectCharacter::AShooterProjectCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
	
	//Set Health
	MaxHealth = 100.f;
	Health = MaxHealth;	

	// set the character speed
	Runningspeed = 450.f;
	Sprintingspeed = 600.f;

	//Sets Interaction Distance and Frequency
	InteractionCheckFrequency = 0.f;
	InteractionCheckDistance = 1000.f;

	// Rotate with Camera
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = true;


	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 350.f;
	GetCharacterMovement()->AirControl = 0.1f;

	//Give the player an inventory with 20 slots, and an 80kg capacity
	PlayerInventory = CreateDefaultSubobject<UInventoryComponent>("PlayerInventory");
	PlayerInventory->SetCapacity(20);
	PlayerInventory->SetWeightCapacity(80.f);

	
	LootPlayerInteraction = CreateDefaultSubobject<UInteractionComponent>("PlayerInteraction");
	LootPlayerInteraction->InteractibleActionText = LOCTEXT("LootPlayerText", "Loot");
	LootPlayerInteraction->InteractibleNameText = LOCTEXT("LootPlayerName", "Player");
	LootPlayerInteraction->SetupAttachment(GetRootComponent());
	LootPlayerInteraction->SetActive(false, true);
	LootPlayerInteraction->bAutoActivate = false;

	// Create a camera boom and attach it to Head socket.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh(), FName("sHead"));
	CameraBoom->TargetArmLength = 0.f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	//Modular Character
	HelmetMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Helmet, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HelmetMesh")));
	ChestMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Chest, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ChestMesh")));
	LegsMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Legs, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LegsMesh")));
	FeetMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Feet, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FeetMesh")));
	VestMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Vest, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VestMesh")));
	HandsMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Hands, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandsMesh")));
	BackpackMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Backpack, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BackpackMesh")));

	//Tell all the body meshes to use the head mesh for animation
	for (auto& PlayerMesh : PlayerMeshes)
	{
		USkeletalMeshComponent* MeshComponent = PlayerMesh.Value;
		MeshComponent->SetupAttachment(GetMesh());
		MeshComponent->SetMasterPoseComponent(GetMesh());
	}

	PlayerMeshes.Add(EEquippableSlot::EIS_Head, GetMesh());

	GetMesh()->SetOwnerNoSee(true);

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
}


void AShooterProjectCharacter::SetLootSource(UInventoryComponent* NewLootSource)
{
	//If the item we're looting gets destroyed, we need to tell the client to remove their loot screen
	if (NewLootSource && NewLootSource->GetOwner())
	{
		NewLootSource->GetOwner()->OnDestroyed.AddUniqueDynamic(this, &AShooterProjectCharacter::OnLootSourceOwnerDestroyed);
	}
	
	if (HasAuthority())
	{
		if (NewLootSource)
		{
			if (AShooterProjectCharacter* Character = Cast<AShooterProjectCharacter>(NewLootSource->GetOwner()))
			{
				Character->SetLifeSpan(120.f);
			}
		}
		
		LootSource = NewLootSource;
	}
	else
	{
		ServerSetLootSource(NewLootSource);
	}
}

bool AShooterProjectCharacter::IsLooting() const
{
	return LootSource != nullptr;
}

void AShooterProjectCharacter::BeginLootingPlayer(AShooterProjectCharacter* Character)
{
	if (Character)
	{
		Character->SetLootSource(PlayerInventory);
	}
}

void AShooterProjectCharacter::ServerSetLootSource_Implementation(UInventoryComponent* NewLootSource)
{
	SetLootSource(NewLootSource);
}

bool AShooterProjectCharacter::ServerSetLootSource_Validate(UInventoryComponent* NewLootSource)
{
	return true;
}

void AShooterProjectCharacter::OnLootSourceOwnerDestroyed(AActor* DestroyedActor)
{
	//Remove loot source
	if (HasAuthority() && LootSource && DestroyedActor == LootSource->GetOwner())
	{
		ServerSetLootSource(nullptr);
	}
}

void AShooterProjectCharacter::OnRep_LootSource()
{
	//Bring up or hide the looting menu
	if (AShooterProjectPlayerController* PC = Cast<AShooterProjectPlayerController>(GetController()))
	{
		if (PC->IsLocalPlayerController())
		{
			if (LootSource)
			{
				PC->ShowLootMenu(LootSource);
			}
			else
			{
				PC->HideLootMenu();
			}
		}
	}
}

void AShooterProjectCharacter::LootItem(UItem* ItemToGive)
{
	if (HasAuthority())
	{
		if (PlayerInventory && LootSource && ItemToGive && LootSource->HasItem(ItemToGive->GetClass(), ItemToGive->GetQuantity()))
		{
			const FItemAddResult AddResult = PlayerInventory->TryAddItem(ItemToGive);

			if (AddResult.ActualAmountGiven > 0)
			{
				LootSource->ConsumeItem(ItemToGive, AddResult.ActualAmountGiven);
			}
			else
			{
				if (AShooterProjectPlayerController* PC = Cast<AShooterProjectPlayerController>(GetController()))
				{
					PC->ShowNotification(AddResult.ErrorText);
				}
			}
		}
	}
	else
	{
		ServerLootItem(ItemToGive);
	}
	
}

void AShooterProjectCharacter::ServerLootItem_Implementation(UItem* ItemToLoot)
{
	LootItem(ItemToLoot);
}

bool AShooterProjectCharacter::ServerLootItem_Validate(UItem* ItemToLoot)
{
	return true;
}

// Called when the game starts or when spawned
void AShooterProjectCharacter::BeginPlay()
{
	Super::BeginPlay();

	//Bind interact to BeginLooting-function that sets Player Inventory as the LootSource.
	LootPlayerInteraction->OnInteract.AddDynamic(this, &AShooterProjectCharacter::BeginLootingPlayer);

	//Try to display the players platform name on their loot card
	if (APlayerState* PS = GetPlayerState())
	{
		LootPlayerInteraction->SetInteractableText(FText::FromString(PS->GetPlayerName()));
	}

	//When the player spawns in, they have no items equipped, so cache thise items (That way, if a player uneqipps an item we can set the mesh back to the standard
	for (auto& PlayerMesh : PlayerMeshes)
	{
		NakedMeshes.Add(PlayerMesh.Key, PlayerMesh.Value->SkeletalMesh);
	}
}


void AShooterProjectCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const bool bIsInteractingOnServer = (HasAuthority() && IsInteracting());

	if ((!HasAuthority() || bIsInteractingOnServer) && GetWorld()->TimeSince(InteractionData.LastInteractionCheckTime) > InteractionCheckFrequency)
	{
		PerformInteractionCheck();
	}
}

void AShooterProjectCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterProjectCharacter, LootSource);
	DOREPLIFETIME(AShooterProjectCharacter, Health);
	//DOREPLIFETIME_CONDITION(AShooterProjectCharacter, Health, COND_OwnerOnly);
}


void AShooterProjectCharacter::Restart()
{
	Super::Restart();

	if (AShooterProjectPlayerController* PC = Cast<AShooterProjectPlayerController>(GetController()))
	{
		PC->ShowInGameUI();
	}
}

float AShooterProjectCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	return DamageAmount;
}

//////////////////////////////////////////////////////////////////////////
// Input

void AShooterProjectCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AShooterProjectCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AShooterProjectCharacter::StopFire);
	
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AShooterProjectCharacter::StartCrouching);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AShooterProjectCharacter::StopCrouching);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AShooterProjectCharacter::BeginInteract);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &AShooterProjectCharacter::EndInteract);

	PlayerInputComponent->BindAction("Prone", IE_Pressed, this, &AShooterProjectCharacter::StartProning);
	PlayerInputComponent->BindAction("Prone", IE_Released, this, &AShooterProjectCharacter::StopProning);

	PlayerInputComponent->BindAction("NextStance", IE_Pressed, this, &AShooterProjectCharacter::NextPawnState);
	PlayerInputComponent->BindAction("PrevStance", IE_Pressed, this, &AShooterProjectCharacter::PrevPawnState);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterProjectCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterProjectCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterProjectCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterProjectCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AShooterProjectCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AShooterProjectCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AShooterProjectCharacter::OnResetVR);
}


void AShooterProjectCharacter::PerformInteractionCheck()
{

	if (GetController() == nullptr)
	{
		return;
	}

	InteractionData.LastInteractionCheckTime = GetWorld()->GetTimeSeconds();

	FVector EyesLoc;
	FRotator EyesRot;

	GetController()->GetPlayerViewPoint(EyesLoc, EyesRot);

	FVector TraceStart = EyesLoc;
	FVector TraceEnd = (EyesRot.Vector() * InteractionCheckDistance) + TraceStart;
	FHitResult TraceHit;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); //Ignore the player

	if (GetWorld()->LineTraceSingleByChannel(TraceHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		//Check if linetrace hits an interactable object
		if (TraceHit.GetActor())
		{
			if (UInteractionComponent* InteractionComponent = Cast<UInteractionComponent>(TraceHit.GetActor()->GetComponentByClass(UInteractionComponent::StaticClass())))
			{
				float Distance = (TraceStart - TraceHit.ImpactPoint).Size();

				if (InteractionComponent != GetInteractable() && Distance <= InteractionComponent->InteractionDistance)
				{
					FoundNewInteractable(InteractionComponent);			
				}
				else if (Distance > InteractionComponent->InteractionDistance && GetInteractable())
				{
					CouldntFindInteractable();
				}

				return;
			}
		}
	}

	CouldntFindInteractable();
}


void AShooterProjectCharacter::CouldntFindInteractable()
{
	//We've lost focus on an interactable. Clear the timer.
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_Interact))
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_Interact);
	}

	//Tell the interactable we've stopped focusing on it, and clear the current interactable.
	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->EndFocus(this);

		if (InteractionData.bInteractHeld)
		{
			EndInteract();
		}
	}

	InteractionData.ViewedInteractionComponent = nullptr;
}


void AShooterProjectCharacter::FoundNewInteractable(UInteractionComponent* Interactable)
{
	EndInteract();

	if (UInteractionComponent* OLDInteractable = GetInteractable())
	{
		OLDInteractable->EndFocus(this);
	}
	InteractionData.ViewedInteractionComponent = Interactable;
	Interactable->BeginFocus(this);
}


void AShooterProjectCharacter::BeginInteract()
{
	if (!HasAuthority())
	{
		ServerBeginInteract();
	}

	/**As an optimization, the server only checks that we're looking at an item once we begin interacting with it.
	This saves server doing a check every tick for an interactable Item. The Exeption is a non-instant interact.
	In this case, the server will check every tick for the duration of the interact */
	if (HasAuthority())
	{
		PerformInteractionCheck();
	}

	InteractionData.bInteractHeld = true;

	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->BeginInteract(this);

		//If InteractionTime = 0 It will instantly interact
		if (FMath::IsNearlyZero(Interactable->InteractionTime))
		{
			Interact();
		}
		else
		{
			//If InteractionTime > 0 = TimerHandle_Interact will count the time until it reaches the needed InteractionTime
			GetWorldTimerManager().SetTimer(TimerHandle_Interact, this, &AShooterProjectCharacter::Interact, Interactable->InteractionTime, false);
		}
	}
}


void AShooterProjectCharacter::EndInteract()
{
	if (!HasAuthority())
	{
		ServerEndInteract();
	}

	InteractionData.bInteractHeld = false;

	//Clears the timer if Interaction Key is Release before InteractionTime
	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);

	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->EndInteract(this);
	}
}


void AShooterProjectCharacter::ServerBeginInteract_Implementation()
{
	BeginInteract();
}


bool AShooterProjectCharacter::ServerBeginInteract_Validate()
{
	return true;
}


void AShooterProjectCharacter::ServerEndInteract_Implementation()
{
	EndInteract();
}


bool AShooterProjectCharacter::ServerEndInteract_Validate()
{
	return true;
}


void AShooterProjectCharacter::Interact()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);

	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->Interact(this);
	}
}


bool AShooterProjectCharacter::IsInteracting() const
{
	return GetWorldTimerManager().IsTimerActive(TimerHandle_Interact);
}


float AShooterProjectCharacter::GetRemainingInteractTime() const
{
	return GetWorldTimerManager().GetTimerRemaining(TimerHandle_Interact);
}


void AShooterProjectCharacter::UseItem(class UItem* Item)
{
	//If not server, make the server run the function again.
	if (GetLocalRole() < ROLE_Authority && Item)
	{
		ServerUseItem(Item);
	}

	//If server, make sure the inventory and item exist
	if (HasAuthority())
	{
		if (PlayerInventory && !PlayerInventory->FindItem(Item))
		{
			return;
		}
	}

	//Runs the Use-function on both client and server
	if (Item)
	{
		Item->Use(this);
	}
}


void AShooterProjectCharacter::ServerUseItem_Implementation(class UItem* Item)
{
	UseItem(Item);
}


bool AShooterProjectCharacter::ServerUseItem_Validate(class UItem* Item)
{
	return true;
}


void AShooterProjectCharacter::DropItem(class UItem* Item, const int32 Quantity)
{
	if (PlayerInventory && Item && PlayerInventory->FindItem(Item))
	{
		if (GetLocalRole() < ROLE_Authority)
		{
			ServerDropItem(Item, Quantity);
			return;
		}

		if (HasAuthority())
		{
			const int32 ItemQuantity = Item->GetQuantity();
			const int32 DroppedQuantity = PlayerInventory->ConsumeItem(Item, Quantity);

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.bNoFail = true;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			FVector SpawnLocation = GetActorLocation();
			SpawnLocation.Z -= GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

			FTransform SpawnTransform(GetActorRotation(), SpawnLocation);

			ensure(PickupClass);

			APickup* Pickup = GetWorld()->SpawnActor<APickup>(PickupClass, SpawnTransform, SpawnParams);
			Pickup->InitializePickup(Item->GetClass(), DroppedQuantity);
		}
	}
}


void AShooterProjectCharacter::ServerDropItem_Implementation(class UItem* Item, const int32 Quantity)
{
	DropItem(Item, Quantity);
}


bool AShooterProjectCharacter::ServerDropItem_Validate(class UItem* Item, const int32 Quantity)
{
	return true;
}


bool AShooterProjectCharacter::EquipItem(class UEquippableItem* Item)
{
	EquippedItems.Add(Item->Slot, Item);
	OnEquippedItemsChanged.Broadcast(Item->Slot, Item);
	return true;
}


bool AShooterProjectCharacter::UnequipItem(class UEquippableItem* Item)
{
	if (Item)
	{
		if (EquippedItems.Contains(Item->Slot))
		{
			if (Item == *EquippedItems.Find(Item->Slot))
			{
				EquippedItems.Remove(Item->Slot);
				OnEquippedItemsChanged.Broadcast(Item->Slot, nullptr);
				return true;
			}
		}
	}
	return false;
}


void AShooterProjectCharacter::EquipGear(class UGearItem* Gear)
{
	if (USkeletalMeshComponent* GearMesh = *PlayerMeshes.Find(Gear->Slot))
	{
		GearMesh->SetSkeletalMesh(Gear->Mesh);
		GearMesh->SetMaterial(GearMesh->GetMaterials().Num() - 1, Gear->MaterialInstance);
	}
}


void AShooterProjectCharacter::UnequipGear(const EEquippableSlot Slot)
{
	if (USkeletalMeshComponent* EquippableMesh = *PlayerMeshes.Find(Slot))
	{
		if (USkeletalMesh* BodyMesh = *NakedMeshes.Find(Slot))
		{
			EquippableMesh->SetSkeletalMesh(BodyMesh);

			//Put the materials back on the body mesh (Since gear may have applied a different material
			for (int32 i = 0; i < BodyMesh->Materials.Num(); ++i)
			{
				if (BodyMesh->Materials.IsValidIndex(i))
				{
					EquippableMesh->SetMaterial(i, BodyMesh->Materials[i].MaterialInterface);
				}
			}
		}
		else
		{
			//For some gear like backpacks, there is no naked mesh
			EquippableMesh->SetSkeletalMesh(nullptr);
		}
	}
}


class USkeletalMeshComponent* AShooterProjectCharacter::GetSlotSkeletalMeshComponent(const EEquippableSlot Slot)
{
	if (PlayerMeshes.Contains(Slot))
	{
		return *PlayerMeshes.Find(Slot);
	}
	return nullptr;
}


// Inventory GUI Selection - UP
void AShooterProjectCharacter::NextInventoryItem()
{

}


// Inventory GUI Selection - Down
void AShooterProjectCharacter::PrevInventoryItem()
{

}


void AShooterProjectCharacter::StartCrouching()
{
	CurrentState = EPawnStates::CROUCH;
	Crouch();
}


void AShooterProjectCharacter::StopCrouching()
{
	CurrentState = EPawnStates::STAND;
	UnCrouch();
}


void AShooterProjectCharacter::StartProning()
{
	CurrentState = EPawnStates::PRONE;
}


void AShooterProjectCharacter::StopProning()
{
	CurrentState = EPawnStates::STAND;
}

float AShooterProjectCharacter::ModifyHealth(const float Delta)
{
	const float OldHealth = Health;

	Health = FMath::Clamp<float>(Health + Delta, 0.f, MaxHealth);

	return Health - OldHealth;
}

void AShooterProjectCharacter::OnRep_Health(float OldHealth)
{
	OnHealthModified(Health - OldHealth);
}

void AShooterProjectCharacter::StartFire()
{
	BeginMeleeAttach();
}

void AShooterProjectCharacter::StopFire()
{
	
}

void AShooterProjectCharacter::BeginMeleeAttach()
{
	//Generate a random number between 1 and 2.
	int MontageSectionIndex = rand() % 3 + 1;

	// Fstring animation Section
	FString MontageSection = "start_" + FString::FromInt(MontageSectionIndex);
	
	PlayAnimMontage(MeleeFistAttackMontage, 1.f, FName(*MontageSection));
}

void AShooterProjectCharacter::ServerProcessMeleeHit_Implementation(const FHitResult& MeleeHit)
{
	if (GetWorld()->TimeSince(LastMeleeAttackTime) > MeleeAttackMontage->GetPlayLength())
	{
		MulticastPlayMeleeFX();

		if (AShooterProjectCharacter* HitPlayer = Cast<AShooterProjectCharacter>(MeleeHit.GetActor()))
		{
			UGameplayStatics::ApplyPointDamage(HitPlayer, MeleeAttachDamage, (MeleeHit.TraceStart - MeleeHit.TraceEnd).GetSafeNormal(), MeleeHit, GetController(), this, MeleeDamageType);
		}
	}
	
	LastMeleeAttackTime = GetWorld()->GetDeltaSeconds();
}

void AShooterProjectCharacter::MulticastPlayMeleeFX_Implementation()
{
	if (!IsLocallyControlled())
	{
		PlayAnimMontage(MeleeAttackMontage);
	}
}

void AShooterProjectCharacter::Suicide(FDamageEvent const& DamageEvent, const AActor* DamageCauser)
{
	Killer = this;
	OnRep_Killer();
}

void AShooterProjectCharacter::KilledByPlayer(FDamageEvent const& DamageEvent, const AShooterProjectPlayerController* EventInstigator, const AActor* DamageCauser)
{
	Killer = Cast<AShooterProjectCharacter>(EventInstigator->GetPawn());
	OnRep_Killer();
}


void AShooterProjectCharacter::OnRep_Killer()
{
	//Remove player from World after 20 seconds
	SetLifeSpan(20.f);

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	SetReplicateMovement(false);

	TurnOff();

	//Activates Looting after player is dead
	LootPlayerInteraction->Activate();

	//Unequip all equipped items so they can be looted
	if (HasAuthority())
	{
		TArray<UEquippableItem*> Equippables;
		EquippedItems.GenerateValueArray(Equippables);

		for (auto& EquippedItem : Equippables)
		{
			EquippedItem->SetEquipped(false);
		}
	}
	//Show Deathscreen
	if (IsLocallyControlled())
	{
		if (AShooterProjectPlayerController* PC = Cast<AShooterProjectPlayerController>(GetController()))
		{
			PC->ShowDeathScreen(Killer);
		}
	}
}


// Next AnimBP State
void AShooterProjectCharacter::NextPawnState()
{
	if (CurrentState == EPawnStates::STAND)
	{
		CurrentState = EPawnStates::CROUCH;
	}
	else if (CurrentState == EPawnStates::CROUCH)
	{
		CurrentState = EPawnStates::PRONE;
	}
}


// Prev AnimBP State
void AShooterProjectCharacter::PrevPawnState()
{
	if (CurrentState == EPawnStates::PRONE)
	{
		CurrentState = EPawnStates::CROUCH;
	}
	else if (CurrentState == EPawnStates::CROUCH)
	{
		CurrentState = EPawnStates::STAND;
	}
}


void AShooterProjectCharacter::OnResetVR()
{
	// If ShooterProject is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in ShooterProject.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}


void AShooterProjectCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}


void AShooterProjectCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}


void AShooterProjectCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}


void AShooterProjectCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}


void AShooterProjectCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}


void AShooterProjectCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

#undef LOCTEXT_NAMESPACE
