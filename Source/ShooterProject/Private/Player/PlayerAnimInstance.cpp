// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerAnimInstance.h"

#include "GameFramework/PawnMovementComponent.h"
#include "Player/ShooterProjectCharacter.h"
#include "Engine.h"

UPlayerAnimInstance::UPlayerAnimInstance()
{
	bIsInAir = false;
	bBlockAimoffset = false;
	Speed = 0.f;
	Direction = 0.f;
}


void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Cache pawn
	Owner = TryGetPawnOwner();
}


void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// ensure that owner is valid
	if (!Owner)
	{
		return;
	}

	if (Owner->IsA(AShooterProjectCharacter::StaticClass()))
	{
		AShooterProjectCharacter* PlayerCharacter = Cast<AShooterProjectCharacter>(Owner);
		if (PlayerCharacter)
		{
			bWeaponEquipped = PlayerCharacter->IsHoldingWeapon();
			bIsInAir = PlayerCharacter->GetMovementComponent()->IsFalling();
			bIsLocallyControlled = PlayerCharacter->IsLocallyControlled();
			Speed = PlayerCharacter->GetVelocity().Size();
			Velocity = PlayerCharacter->GetVelocity();
			ActorRotation = PlayerCharacter->GetActorRotation();
			Direction = CalculateDirection(Velocity, ActorRotation);

			//Prone fix
			bBlockAimoffset = PlayerCharacter->ProneFix == EPawnState::Prone;

			// Stance States.
			bIsStanding = PlayerCharacter->CurrentPawnState == EPawnState::Stand;
			bIsCrouching = PlayerCharacter->CurrentPawnState == EPawnState::Crouch;
			bIsProning = PlayerCharacter->CurrentPawnState == EPawnState::Prone;
			
			// Weapon AimOffset States. Changing the upper-body basepose and selecting the correct aimoffset.
			bIsResting = PlayerCharacter->CurrentOffsetState == EWeaponOffsetState::Resting;
			bIsReady = PlayerCharacter->CurrentOffsetState == EWeaponOffsetState::Ready;
			bIsAiming = PlayerCharacter->CurrentOffsetState == EWeaponOffsetState::Aiming;
		}
	}
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