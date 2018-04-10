// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "StreamDataSingleton.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class STREAM_API UStreamDataSingleton : public UObject
{
	GENERATED_BODY()
public:
	UStreamDataSingleton(const FObjectInitializer& ObjectInitializer);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Gameplay Streaming")
	bool CanStream;
	
};
