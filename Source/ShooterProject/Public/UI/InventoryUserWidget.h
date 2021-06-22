// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/ShooterProjectCharacter.h"
#include "Items/WeaponClass.h"
#include "Runtime/UMG/Public/UMG.h"
#include "InventoryUserWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;

USTRUCT(BlueprintType)
struct FCachedInventoryData
{
	GENERATED_BODY()

	//CACHED THUMBNAILS 
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	UTexture2D* Primary_Thumbnail;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	UTexture2D* Secondary_Thumbnail;
};

UCLASS()
class SHOOTERPROJECT_API UInventoryUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UInventoryUserWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets")
	FVector2D TextureSize;
	
	//Inventory Selection Buttons
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UButton* PrimaryButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UButton* SecondaryButton;

};
