// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayStreamer.h"

#include "FFMuxer.h"

// Sets default values
AGameplayStreamer::AGameplayStreamer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	mMuxer = new FFMuxer();
}

AGameplayStreamer::~AGameplayStreamer()
{
	mStreamStarted = false;
	if (mWorkerThread)
	{
		mWorkerThread->join();
	}
	delete mWorkerThread;
	delete mMuxer;
}

// Called when the game starts or when spawned
void AGameplayStreamer::BeginPlay()
{
	Super::BeginPlay();
	
}


// Called every frame
void AGameplayStreamer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// in this stage we initializing ffmpeg
// starting filling video buffer from Viewport->draw
// starting filling audio buffer from audio list callbacks(maybe fmod integration)
void AGameplayStreamer::StartStream()
{
	if (!mStreamStarted)
	{
		UE_LOG(LogTemp, Warning, TEXT("Starting stream..."));
		mStreamStarted = true;
		mWorkerThread = new std::thread(&AGameplayStreamer::StreamingLogic, this);
	}
}

// stops stream, releases data
void AGameplayStreamer::StopStream()
{
	UE_LOG(LogTemp, Warning, TEXT("Stop stream..."));
}
// pauses stream, but not realeasing data
void AGameplayStreamer::PauseStream()
{
	UE_LOG(LogTemp, Warning, TEXT("Pause stream..."));
}

void AGameplayStreamer::StreamingLogic()
{
	while (mStreamStarted)
	{
		mMuxer->Mux();
	}
}

