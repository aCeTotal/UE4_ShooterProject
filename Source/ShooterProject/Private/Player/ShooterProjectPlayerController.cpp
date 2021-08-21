// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ShooterProjectPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Player/ShooterProjectCharacter.h"


AShooterProjectPlayerController::AShooterProjectPlayerController()
{

}

void AShooterProjectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("Reload", IE_Pressed, this, &AShooterProjectPlayerController::StartReload);

	InputComponent->BindAction("Fire", IE_Pressed, this, &AShooterProjectPlayerController::OnStartFire);
	InputComponent->BindAction("Fire", IE_Released, this, &AShooterProjectPlayerController::OnStopFire);
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

void AShooterProjectPlayerController::OnStartFire()
{
	if (AShooterProjectCharacter* ShooterCharacter = Cast<AShooterProjectCharacter>(GetPawn()))
	{
		if (VerticalHotbarBoxRef && VerticalHotbarBoxRef->IsVisible())
		{
		}
		else
		{
			ShooterCharacter->StartFire();
		}
	}	
}

void AShooterProjectPlayerController::OnStopFire()
{
	if (AShooterProjectCharacter* ShooterCharacter = Cast<AShooterProjectCharacter>(GetPawn()))
	{
		ShooterCharacter->StopFire();
	}	
}

void AShooterProjectPlayerController::ClientShowNotification_Implementation(const FText& Message)
{
	ShowNotification(Message);
}
