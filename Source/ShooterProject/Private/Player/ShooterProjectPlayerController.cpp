// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ShooterProjectPlayerController.h"

#include "Player/ShooterProjectCharacter.h"


AShooterProjectPlayerController::AShooterProjectPlayerController()
{

}

void AShooterProjectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAxis("Turn", this, &AShooterProjectPlayerController::Turn);
	InputComponent->BindAxis("LookUp", this, &AShooterProjectPlayerController::LookUp);

	InputComponent->BindAction("Reload", IE_Pressed, this, &AShooterProjectPlayerController::StartReload);
}

void AShooterProjectPlayerController::Respawn()
{
	UnPossess();
	ChangeState(NAME_Inactive);

	if (!HasAuthority())
	{
		ServerRespawn();
	}
	else
	{
		ServerRestartPlayer();
	}
}

void AShooterProjectPlayerController::ServerRespawn_Implementation()
{
	Respawn();
}

bool AShooterProjectPlayerController::ServerRespawn_Validate()
{
	return true;
}

void AShooterProjectPlayerController::ApplyRecoil(const FVector2D& RecoilAmount, const float RecoilSpeed, const float RecoilResetSpeed, TSubclassOf<UMatineeCameraShake> Shake)
{
	if (IsLocalPlayerController())
	{
		if (PlayerCameraManager)
		{
			PlayerCameraManager->StartCameraShake(Shake);
		}

		RecoilBumpAmount += RecoilAmount;
		RecoilResetAmount += -RecoilAmount;

		CurrentRecoilSpeed = RecoilSpeed;
		CurrentRecoilResetSpeed = RecoilResetSpeed;

		LastRecoilTime = GetWorld()->GetTimeSeconds();
	}
}

void AShooterProjectPlayerController::Turn(float Rate)
{
	//If the player has moved their camera to compensate for recoil we need this to cancel out the recoil reset effect
	if (!FMath::IsNearlyZero(RecoilResetAmount.X, 0.01f))
	{
		if (RecoilResetAmount.X > 0.f && Rate > 0.f)
		{
			RecoilResetAmount.X = FMath::Max(0.f, RecoilResetAmount.X - Rate);
		}
		else if (RecoilResetAmount.X < 0.f && Rate < 0.f)
		{
			RecoilResetAmount.X = FMath::Min(0.f, RecoilResetAmount.X - Rate);
		}
	}

	//Apply the recoil over several frames
	if (!FMath::IsNearlyZero(RecoilBumpAmount.X, 0.1f))
	{
		FVector2D LastCurrentRecoil = RecoilBumpAmount;
		RecoilBumpAmount.X = FMath::FInterpTo(RecoilBumpAmount.X, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilSpeed);

		AddYawInput(LastCurrentRecoil.X - RecoilBumpAmount.X);
	}

	//Slowly reset back to center after recoil is processed
	FVector2D LastRecoilResetAmount = RecoilResetAmount;
	RecoilResetAmount.X = FMath::FInterpTo(RecoilResetAmount.X, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilResetSpeed);
	AddYawInput(LastRecoilResetAmount.X - RecoilResetAmount.X);

	AddYawInput(Rate);
}

void AShooterProjectPlayerController::LookUp(float Rate)
{
	if (!FMath::IsNearlyZero(RecoilResetAmount.Y, 0.01f))
	{
		if (RecoilResetAmount.Y > 0.f && Rate > 0.f)
		{
			RecoilResetAmount.Y = FMath::Max(0.f, RecoilResetAmount.Y - Rate);
		}
		else if (RecoilResetAmount.Y < 0.f && Rate < 0.f)
		{
			RecoilResetAmount.Y = FMath::Min(0.f, RecoilResetAmount.Y - Rate);
		}
	}

	//Apply the recoil over several frames
	if (!FMath::IsNearlyZero(RecoilBumpAmount.Y, 0.01f))
	{
		FVector2D LastCurrentRecoil = RecoilBumpAmount;
		RecoilBumpAmount.Y = FMath::FInterpTo(RecoilBumpAmount.Y, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilSpeed);

		AddPitchInput(LastCurrentRecoil.Y - RecoilBumpAmount.Y);
	}

	//Slowly reset back to center after recoil is processed
	FVector2D LastRecoilResetAmount = RecoilResetAmount;
	RecoilResetAmount.Y = FMath::FInterpTo(RecoilResetAmount.Y, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilResetSpeed);
	AddPitchInput(LastRecoilResetAmount.Y - RecoilResetAmount.Y);

	AddPitchInput(Rate);
}

void AShooterProjectPlayerController::StartReload()
{
	if (AShooterProjectCharacter* ShooterCharacter = Cast<AShooterProjectCharacter>(GetPawn()))
	{
		if (ShooterCharacter->IsAlive())
		{
			ShooterCharacter->StartReload();
		}
		else
		{
			Respawn();
		}
	}
}

void AShooterProjectPlayerController::ClientShowNotification_Implementation(const FText& Message)
{
	ShowNotification(Message);
}
