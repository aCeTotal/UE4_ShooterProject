// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ShooterProjectPlayerController.h"

#include "Player/ShooterProjectCharacter.h"


AShooterProjectPlayerController::AShooterProjectPlayerController()
{

}

void AShooterProjectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

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
