// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "StreamGameMode.h"
#include "StreamHUD.h"
#include "StreamCharacter.h"
#include "UObject/ConstructorHelpers.h"

AStreamGameMode::AStreamGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AStreamHUD::StaticClass();
}
