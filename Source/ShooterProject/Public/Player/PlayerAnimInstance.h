// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERPROJECT_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool IsInAir;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float Direction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FVector Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FRotator ActorRotation;

	// Animation State Checks
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	bool bIsStanding;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	bool bIsCrouching;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	bool bIsProning;

private:

	float CalculateDirection(const FVector& PlayerVelocity, const FRotator& PlayerRotation) const;

public:
	UPlayerAnimInstance();

	virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:

	APawn* Owner;
};
