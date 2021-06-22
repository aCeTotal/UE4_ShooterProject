// Copyright Epic Games, Inc. All Rights Reserved.

#include "Framework/ShooterProjectGameMode.h"
#include "Player/ShooterProjectCharacter.h"
#include "UObject/ConstructorHelpers.h"

AShooterProjectGameMode::AShooterProjectGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Blueprints/PlayerCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
