// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Async.h"
#include "GameFramework/Actor.h"
#include "GameplayStreamer.generated.h"

UCLASS()
class STREAM_API AGameplayStreamer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGameplayStreamer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "GameplayStreaming")
	void StartStream();

	UFUNCTION(BlueprintCallable, Category = "GameplayStreaming")
	void StopStream();

	UFUNCTION(BlueprintCallable, Category = "GameplayStreaming")
	void PauseStream();
	
};
