// Fill out your copyright notice in the Description page of Project Settings.

#include "AudioEvent.h"

// Sets default values
AAudioEvent::AAudioEvent()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAudioEvent::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAudioEvent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AAudioEvent::FillAudioBuffer(FString AudioTrackName)
{
	UE_LOG(LogTemp, Warning, TEXT("Filling buffer with given audio track, %s"), *AudioTrackName);

	// AudioBuffer::Add(AudioManager::get(AudioTrackName));
}

