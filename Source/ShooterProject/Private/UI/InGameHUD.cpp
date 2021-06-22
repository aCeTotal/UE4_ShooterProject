// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InGameHUD.h"

AInGameHUD::AInGameHUD()
{

}

void AInGameHUD::BeginPlay()
{
	Super::BeginPlay();

	if (HUDWidgetClass)
	{
		InventoryWidget = CreateWidget<UInventoryUserWidget>(GetWorld(), HUDWidgetClass);
		if (InventoryWidget)
		{
			InventoryWidget->AddToViewport();
		}
	}
}

void AInGameHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AInGameHUD::DrawHUD()
{
	Super::DrawHUD();
}