// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ShooterProjectPlayerController.h"


AShooterProjectPlayerController::AShooterProjectPlayerController()
{

}

void AShooterProjectPlayerController::ClientShowNotification_Implementation(const FText& Message)
{
	ShowNotification(Message);
}
