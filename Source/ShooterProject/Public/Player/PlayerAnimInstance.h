// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Weapon/Weapon.h"

#include "PlayerAnimInstance.generated.h"


class AShooterProjectCharacter;
class UCurveVector;

/**
 * 
 */
UCLASS()
class SHOOTERPROJECT_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	bool bWeaponOnHip;

	UPROPERTY(BlueprintReadOnly, Category = "Character")
	AShooterProjectCharacter* Character;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FTransform RelativeHandTransform;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FTransform SightTransform;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FTransform FinalHandTransform;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FTransform LeftHandIK;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float AimAlpha;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FVector SwayLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FRotator UnmodifiedRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FTransform TurnSwayTransform;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FRotator OldRotation;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FTransform RecoilTransform;
	FTransform FinalRecoilTransform;

	
	bool bInterpAiming;
	bool bInterpRelativeHand;


	//Recoil
	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	float InterpFinalRecoil_Speed;
	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	float InterpRecoil_Speed;
	
	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	float RecoilLocation_X_Min;
	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	float RecoilLocation_X_Max;
	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	float RecoilLocation_Y_Min;
	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	float RecoilLocation_Y_Max;
	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	float RecoilLocation_Z_Min;
	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	float RecoilLocation_Z_Max;

	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	float RecoilRotation_Pitch_Min;
	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	float RecoilRotation_Pitch_Max;
	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	float RecoilRotation_Yaw_Min;
	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	float RecoilRotation_Yaw_Max;
	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	float RecoilRotation_Roll_Min;
	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	float RecoilRotation_Roll_Max;

	void GetRecoilValues();

protected:

	//Functions used to get the weapon sight in front of the camera
	void SetSightTransform();
	void SetRelativeHandTransform();
	void SetFinalHandTransform();
	void SetLeftHandTransform();

	void InterpAiming(float DeltaSeconds);
	void InterpRelativeHand(float DeltaSeconds);

	void MoveVectorCurve(float DeltaSeconds);
	void SwayRotationOffset(float DeltaSeconds);

	void InterpFinalRecoil(float DeltaSeconds);
	void InterpRecoil(float DeltaSeconds);

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

	void CycledWeaponSight();

	UFUNCTION(BlueprintCallable, Category = Weapon)
	void Recoil();

private:

	float CalculateDirection(const FVector& PlayerVelocity, const FRotator& PlayerRotation) const;

public:
	UPlayerAnimInstance();

	virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
