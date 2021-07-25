// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Weapon/Weapon.h"

#include "PlayerAnimInstance.generated.h"


class AShooterProjectCharacter;

/**
 * 
 */
UCLASS()
class SHOOTERPROJECT_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, Category = "Character")
	AShooterProjectCharacter* Character;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FTransform RelativeHandTransform;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FTransform SightTransform;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float AimAlpha;

	bool bInterpAiming;

protected:

	//Functions used to get the weapon sight in front of the camera
	void SetSightTransform();
	void SetRelativeHandTransform();

	void InterpAiming();

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsInAir;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bBlockAimoffset;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsLocallyControlled;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Direction;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FVector Velocity;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FRotator ActorRotation;

	// Animation State Checks
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bWeaponEquipped;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsStanding;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsCrouching;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsProning;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsResting;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsAiming;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsReady;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	AWeapon* CurrentWeapon;

public:

	void SetAiming(bool bNewAiming);

private:

	float CalculateDirection(const FVector& PlayerVelocity, const FRotator& PlayerRotation) const;

public:
	UPlayerAnimInstance();

	virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
