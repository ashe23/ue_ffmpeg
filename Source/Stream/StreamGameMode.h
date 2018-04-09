// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StreamGameMode.generated.h"

UCLASS(minimalapi)
class AStreamGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AStreamGameMode();
	void BeginPlay() override;

	bool CanStream = false;
};



