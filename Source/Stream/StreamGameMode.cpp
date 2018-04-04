// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "StreamGameMode.h"
#include "StreamHUD.h"
#include "StreamCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine.h"

AStreamGameMode::AStreamGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	//static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	//DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AStreamHUD::StaticClass();
}

void AStreamGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (GEngine)
	{
		UE_LOG(LogTemp, Warning, TEXT("Game mode begin played called. Game default configs setting here."));
		UGameUserSettings* MyGameSettings = GEngine->GetGameUserSettings();
		MyGameSettings->SetScreenResolution(FIntPoint(1280, 720));
		MyGameSettings->SetFullscreenMode(EWindowMode::Windowed);
		MyGameSettings->SetVSyncEnabled(false);
		MyGameSettings->ApplySettings(false);
	}
}
