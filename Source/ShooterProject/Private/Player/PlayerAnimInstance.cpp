// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerAnimInstance.h"

#include "GameFramework/PawnMovementComponent.h"
#include "Player/ShooterProjectCharacter.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Curves/CurveVector.h"
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
	AimAlpha = 1.0f;
}


void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Cache pawn
	Character = Cast<AShooterProjectCharacter>(TryGetPawnOwner());

	if (Character && bWeaponEquipped)
	{
		OldRotation = Character->GetControlRotation();
	}
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
			bWeaponOnHip = Character->bWeaponOnHip;
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
	if (Character && !bInterpRelativeHand && bWeaponEquipped)
	{
		SetSightTransform();
		SetRelativeHandTransform();
		SetLeftHandTransform();
	}

	if (Character && bInterpAiming && bWeaponEquipped)
	{
		InterpAiming(DeltaSeconds);
	}

	if (Character && bInterpRelativeHand && bWeaponEquipped)
	{
		InterpRelativeHand(DeltaSeconds);
	}

	if (Character && bWeaponEquipped)
	{
		GetRecoilValues();
		MoveVectorCurve(DeltaSeconds);
		SwayRotationOffset(DeltaSeconds);
		InterpRecoil(DeltaSeconds);
		InterpFinalRecoil(DeltaSeconds);
	}

}


void UPlayerAnimInstance::GetRecoilValues()
{
	InterpRecoil_Speed = CurrentWeapon->InterpRecoil_Speed;
	InterpFinalRecoil_Speed = CurrentWeapon->InterpFinalRecoil_Speed;
	
	//Recoil Location
	RecoilLocation_X_Min = CurrentWeapon->RecoilLocation_X_Min;
	RecoilLocation_X_Max = CurrentWeapon->RecoilLocation_X_Max;
	RecoilLocation_Y_Min = CurrentWeapon->RecoilLocation_Y_Min;
	RecoilLocation_Y_Max = CurrentWeapon->RecoilLocation_Y_Max;
	RecoilLocation_Z_Min = CurrentWeapon->RecoilLocation_Z_Min;
	RecoilLocation_Z_Max = CurrentWeapon->RecoilLocation_Z_Max;

	//Recoil Rotation
	RecoilRotation_Pitch_Min = CurrentWeapon->RecoilRotation_Pitch_Min;
	RecoilRotation_Pitch_Max = CurrentWeapon->RecoilRotation_Pitch_Max;
	RecoilRotation_Yaw_Min = CurrentWeapon->RecoilRotation_Yaw_Min;
	RecoilRotation_Yaw_Max = CurrentWeapon->RecoilRotation_Yaw_Max;
	RecoilRotation_Roll_Min = CurrentWeapon->RecoilRotation_Roll_Min;
	RecoilRotation_Roll_Max = CurrentWeapon->RecoilRotation_Roll_Max;
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

void UPlayerAnimInstance::InterpAiming(float DeltaSeconds)
{
	AimAlpha = UKismetMathLibrary::FInterpTo(AimAlpha, static_cast<float>(bIsAiming), DeltaSeconds, 8.0f);

	if (AimAlpha >= 1.0f || AimAlpha <= 0.0f)
	{
		bInterpAiming = false;
	}
}

void UPlayerAnimInstance::InterpRelativeHand(float DeltaSeconds)
{
	RelativeHandTransform = UKismetMathLibrary::TInterpTo(RelativeHandTransform, FinalHandTransform, DeltaSeconds, 8.0f);

	if (RelativeHandTransform.Equals(FinalHandTransform))
	{
		bInterpRelativeHand = false;
	}
}

void UPlayerAnimInstance::MoveVectorCurve(float DeltaSeconds)
{
	if (!bWeaponOnHip)
	{
		if (Speed == 0.0f)
		{
			if (CurrentWeapon->Aiming_IdleCurve)
			{
				FVector NewVector = CurrentWeapon->Aiming_IdleCurve->GetVectorValue(Character->GetGameTimeSinceCreation());
				SwayLocation = UKismetMathLibrary::VInterpTo(SwayLocation, NewVector, DeltaSeconds, 10.0f);
			}
		}
	
		if (Speed > 0.0f && bIsCrouching)
		{
			if (CurrentWeapon->Aiming_SlowWalkCurve)
			{
				FVector NewVector = CurrentWeapon->Aiming_SlowWalkCurve->GetVectorValue(Character->GetGameTimeSinceCreation());
				SwayLocation = UKismetMathLibrary::VInterpTo(SwayLocation, NewVector, DeltaSeconds, 10.0f);
			}
		}

		if (Speed > 0.0f && !bIsCrouching)
		{
			if (CurrentWeapon->Aiming_FastWalkCurve)
			{
				FVector NewVector = CurrentWeapon->Aiming_FastWalkCurve->GetVectorValue(Character->GetGameTimeSinceCreation());
				SwayLocation = UKismetMathLibrary::VInterpTo(SwayLocation, NewVector, DeltaSeconds, 10.0f);
			}
		}
	}

	if (bWeaponOnHip)
	{
		if (Speed == 0.0f)
		{
			if (CurrentWeapon->Hip_IdleCurve)
			{
				FVector NewVector = CurrentWeapon->Hip_IdleCurve->GetVectorValue(Character->GetGameTimeSinceCreation());
				SwayLocation = UKismetMathLibrary::VInterpTo(SwayLocation, NewVector, DeltaSeconds, 10.0f);
			}
		}
	
		if (Speed > 0.0f && bIsCrouching)
		{
			if (CurrentWeapon->Hip_SlowWalkCurve)
			{
				FVector NewVector = CurrentWeapon->Hip_SlowWalkCurve->GetVectorValue(Character->GetGameTimeSinceCreation());
				SwayLocation = UKismetMathLibrary::VInterpTo(SwayLocation, NewVector, DeltaSeconds, 10.0f);
			}
		}

		if (Speed > 0.0f && !bIsCrouching)
		{
			if (CurrentWeapon->Hip_FastWalkCurve)
			{
				FVector NewVector = CurrentWeapon->Hip_FastWalkCurve->GetVectorValue(Character->GetGameTimeSinceCreation());
				SwayLocation = UKismetMathLibrary::VInterpTo(SwayLocation, NewVector, DeltaSeconds, 10.0f);
			}
		}
	}
}

void UPlayerAnimInstance::SwayRotationOffset(float DeltaSeconds)
{
	FRotator CurrentRotation = Character->GetControlRotation();
	UnmodifiedRotator = UKismetMathLibrary::RInterpTo(UnmodifiedRotator, CurrentRotation - OldRotation, DeltaSeconds, 6.0f);
	FRotator TurnRotation = UnmodifiedRotator;
	TurnRotation.Roll = TurnRotation.Pitch;
	TurnRotation.Pitch = 0.0f;

	TurnRotation.Yaw = FMath::Clamp(TurnRotation.Yaw, -8.0f, 8.0f);
	TurnRotation.Roll = FMath::Clamp(TurnRotation.Roll, -5.0f, 5.0f);

	FVector TurnLocation;
	TurnLocation.X = TurnRotation.Yaw / 4.0f;
	TurnLocation.Z = TurnRotation.Roll / 1.5f;

	TurnSwayTransform.SetLocation(TurnLocation);
	TurnSwayTransform.SetRotation(TurnRotation.Quaternion());

	OldRotation = CurrentRotation;
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

void UPlayerAnimInstance::InterpFinalRecoil(float DeltaSeconds)
{ //Interp to Zero

	FinalRecoilTransform = UKismetMathLibrary::TInterpTo(FinalRecoilTransform, FTransform(), DeltaSeconds, InterpFinalRecoil_Speed);
	
}

void UPlayerAnimInstance::InterpRecoil(float DeltaSeconds)
{ //Interp to FinalRecoilTransform

	RecoilTransform = UKismetMathLibrary::TInterpTo(RecoilTransform, FinalRecoilTransform, DeltaSeconds, InterpRecoil_Speed);
}

void UPlayerAnimInstance::Recoil()
{
	FVector RecoilLoc = FinalRecoilTransform.GetLocation();
	RecoilLoc += FVector(FMath::RandRange(RecoilLocation_X_Min, RecoilLocation_X_Max), FMath::RandRange(RecoilLocation_Y_Min, RecoilLocation_Y_Max), FMath::RandRange(RecoilLocation_Z_Min, RecoilLocation_Z_Max));
	
	FRotator RecoilRot = FinalRecoilTransform.GetRotation().Rotator();
	RecoilRot += FRotator(FMath::RandRange(RecoilRotation_Pitch_Min, RecoilRotation_Pitch_Max), FMath::RandRange(RecoilRotation_Yaw_Min, RecoilRotation_Yaw_Max), FMath::RandRange(RecoilRotation_Roll_Min, RecoilRotation_Roll_Max));

	FinalRecoilTransform.SetRotation(RecoilRot.Quaternion());
	FinalRecoilTransform.SetLocation(RecoilLoc);
}