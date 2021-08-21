// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Components/VerticalBox.h"
#include "GameFramework/PlayerController.h"
#include "ShooterProjectPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERPROJECT_API AShooterProjectPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AShooterProjectPlayerController();

	virtual void SetupInputComponent() override;

	//Call this instead of show notification if on the server
	UFUNCTION(Client, Reliable, BlueprintCallable)
	void ClientShowNotification(const FText& Message);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowNotification(const FText& Message);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowDeathScreen(class AShooterProjectCharacter* Killer);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowLootMenu(const class UInventoryComponent* LootSource);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowInGameUI();

	UFUNCTION(BlueprintImplementableEvent)
	void ClickHotbarButton();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HideLootMenu();

	UFUNCTION(BlueprintImplementableEvent)
	void OnHitPlayer();

	UFUNCTION(BlueprintCallable, Category = "Player Controller")
	void Respawn();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRespawn();

	void StartReload();

	void OnStartFire();
	void OnStopFire();

	UPROPERTY(BlueprintReadWrite)
	UVerticalBox* VerticalHotbarBoxRef;
	
};
