// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/WeaponClass.h"


// Sets default values
AWeaponClass::AWeaponClass()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;

}

// Called when the game starts or when spawned
void AWeaponClass::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeaponClass::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeaponClass::Fire()
{
	AActor* PawnOwner = GetOwner();
	if (PawnOwner)
	{
		FVector MuzzleLocation = WeaponMesh->GetSocketLocation("Muzzle");
		FRotator MuzzleRotation = WeaponMesh->GetSocketRotation("Muzzle");

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, MuzzleRotation);
	}
}

