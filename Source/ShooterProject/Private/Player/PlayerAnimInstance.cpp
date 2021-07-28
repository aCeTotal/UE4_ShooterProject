// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerAnimInstance.h"

#include "GameFramework/PawnMovementComponent.h"
#include "Player/ShooterProjectCharacter.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine.h"

UPlayerAnimInstance::UPlayerAnimInstance()
{
	bIsInAir = false;
	bInterpAiming = false;
	bInterpRelativeHand = false;
	bIsAiming = false;
	bBlockAimoffset = false;
	Speed = 0.0f;
	Direction = 0.0f;
	AimAlpha = 0.0f;
}


void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Cache pawn
	Character = Cast<AShooterProjectCharacter>(TryGetPawnOwner());
}


void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// ensure that Character is valid
	if (!Character)
	{
		return;
	}

	if (Character->IsA(AShooterProjectCharacter::StaticClass()))
	{
		if (Character)
		{
			bWeaponEquipped = Character->IsHoldingWeapon();
			bIsInAir = Character->GetMovementComponent()->IsFalling();
			bIsLocallyControlled = Character->IsLocallyControlled();
			Speed = Character->GetVelocity().Size();
			Velocity = Character->GetVelocity();
			ActorRotation = Character->GetActorRotation();
			Direction = CalculateDirection(Velocity, ActorRotation);

			CurrentWeapon = Character->GetEquippedWeapon();

			//Prone fix - When pawn are leaving prone state, all aimoffsets are disabled.
			bBlockAimoffset = Character->ProneFix == EPawnState::Prone;

			// Stance States.
			bIsStanding = Character->CurrentPawnState == EPawnState::Stand;
			bIsCrouching = Character->CurrentPawnState == EPawnState::Crouch;
			bIsProning = Character->CurrentPawnState == EPawnState::Prone;
			
			// Weapon AimOffset States. Changing the upper-body basepose and selecting the correct aimoffset.
			//bIsResting = Character->IsAiming() == true;
			bIsReady = Character->IsAiming() == false;
			bIsAiming = Character->IsAiming() == true;
		}
	}

	//Call the Transform functions only when a weapon is equipped
	if (!bInterpRelativeHand && bWeaponEquipped)
	{
		SetSightTransform();
		SetRelativeHandTransform();
		SetLeftHandTransform();
	}

	if (bInterpAiming && bWeaponEquipped)
	{
		InterpAiming();
	}

	if (bInterpRelativeHand && bWeaponEquipped)
	{
		InterpRelativeHand();
	}
}


void UPlayerAnimInstance::SetSightTransform()
{
	if (Character)
	{
		FTransform CameraTransform = Character->Get1PCamera()->GetComponentTransform();
		FTransform MeshTransform = Character->Get1PMesh()->GetComponentTransform();

		SightTransform = UKismetMathLibrary::MakeRelativeTransform(CameraTransform, MeshTransform);

		SightTransform.SetLocation(SightTransform.GetLocation() + SightTransform.GetRotation().Vector() * CurrentWeapon->DistanceToSight);
	}
}


void UPlayerAnimInstance::SetRelativeHandTransform()
{
	if (Character && CurrentWeapon && CurrentWeapon->GetCurrentSight())
	{
		FTransform HandTransform = Character->Get1PMesh()->GetSocketTransform(FName("hand_r"));
		FTransform SightSocketTransform = CurrentWeapon->GetCurrentSight()->GetSocketTransform(FName("S_Aim"));

		RelativeHandTransform = UKismetMathLibrary::MakeRelativeTransform(SightSocketTransform, HandTransform);
	}
}

void UPlayerAnimInstance::SetFinalHandTransform()
{
	if (Character && CurrentWeapon && CurrentWeapon->GetCurrentSight())
	{
		FTransform HandTransform = Character->Get1PMesh()->GetSocketTransform(FName("hand_r"));
		FTransform SightSocketTransform = CurrentWeapon->GetCurrentSight()->GetSocketTransform(FName("S_Aim"));

		FinalHandTransform = UKismetMathLibrary::MakeRelativeTransform(SightSocketTransform, HandTransform);
	}
}

void UPlayerAnimInstance::SetLeftHandTransform()
{
	if (Character && bWeaponEquipped)
	{
		FTransform WeaponGripPointTransform = CurrentWeapon->WeaponMesh->GetSocketTransform(FName("LeftHandGripPoint"));
		FTransform RightHandTransform = Character->Get1PMesh()->GetSocketTransform(FName("hand_r"));

		LeftHandIK = UKismetMathLibrary::MakeRelativeTransform(WeaponGripPointTransform, RightHandTransform);
	}
}

void UPlayerAnimInstance::InterpAiming()
{
	AimAlpha = UKismetMathLibrary::FInterpTo(AimAlpha, static_cast<float>(bIsAiming), GetWorld()->GetDeltaSeconds(), 8.0f);

	if (AimAlpha >= 1.0f || AimAlpha <= 0.0f)
	{
		bInterpAiming = false;
	}
}

void UPlayerAnimInstance::InterpRelativeHand()
{
	RelativeHandTransform = UKismetMathLibrary::TInterpTo(RelativeHandTransform, FinalHandTransform, GetWorld()->GetDeltaSeconds(), 8.0f);

	if (RelativeHandTransform.Equals(FinalHandTransform))
	{
		bInterpRelativeHand = false;
	}
}

void UPlayerAnimInstance::SetAiming(bool bNewAiming)
{
	if (bIsAiming != bNewAiming)
	{
		bIsAiming = bNewAiming;
		bInterpAiming = true;
	}	
}

void UPlayerAnimInstance::CycledWeaponSight()
{
	SetFinalHandTransform();
	bInterpRelativeHand = true;
}

//Calculate direction
float UPlayerAnimInstance::CalculateDirection(const FVector& PlayerVelocity, const FRotator& PlayerRotation) const
{
	if (!PlayerVelocity.IsNearlyZero())
	{
		FMatrix RotMatrix = FRotationMatrix(PlayerRotation);
		FVector ForwardVector = RotMatrix.GetScaledAxis(EAxis::X);
		FVector RightVector = RotMatrix.GetScaledAxis(EAxis::Y);
		FVector NormalizedVel = PlayerVelocity.GetSafeNormal2D();

		// get a cos(alpha) of forward vector vs velocity
		float ForwardCosAngle = FVector::DotProduct(ForwardVector, NormalizedVel);
		// now get the alpha and convert to degree
		float ForwardDeltaDegree = FMath::RadiansToDegrees(FMath::Acos(ForwardCosAngle));

		// depending on where right vector is, flip it
		float RightCosAngle = FVector::DotProduct(RightVector, NormalizedVel);
		if (RightCosAngle < 0)
		{
			ForwardDeltaDegree *= -1;
		}

		return ForwardDeltaDegree;
	}

	return 0.f;
}