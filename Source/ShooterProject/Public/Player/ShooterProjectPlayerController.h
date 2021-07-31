// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HideLootMenu();

	UFUNCTION(BlueprintImplementableEvent)
	void OnHitPlayer();

	UFUNCTION(BlueprintCallable, Category = "Player Controller")
	void Respawn();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRespawn();

	/**Applies recoil to the Weapon
	* RecoilAmount the amount to recoil by. X is the yaw, Y is the pitch.
	* RecoilSpeed the speed to bump the Weapon up per second.
	* RecoilResetSpeed, the speed the weapon will return to center at per second after the recoil is finished
	* Shake an optional to play with the recoil
	*/
	void ApplyRecoil(const FVector2D& RecoilAmount, const float RecoilSpeed, const float RecoilResetSpeed, TSubclassOf<class UMatineeCameraShake> Shake = nullptr);

	//The amount of recoil to apply. We store this in a variable as we smoothly apply the recoil over several frames
	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	FVector2D RecoilBumpAmount;

	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	FVector2D RecoilResetAmount;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float CurrentRecoilSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float CurrentRecoilResetSpeed;

	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	float LastRecoilTime;

	void Turn(float Rate);
	void LookUp(float Rate);

	void StartReload();
	
};
