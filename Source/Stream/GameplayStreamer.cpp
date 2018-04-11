// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayStreamer.h"
#include "Runtime/Core/Public/HAL/RunnableThread.h"
#include "StreamDataSingleton.h"
#include "Engine.h"
#include "FFMuxer.h"

MuxerWorker* MuxerWorker::Runnable = nullptr;

// Sets default values
AGameplayStreamer::AGameplayStreamer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;	

	SetGameSingletonClass();
}

AGameplayStreamer::~AGameplayStreamer()
{
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
	UE_LOG(LogTemp, Warning, TEXT("Starting stream..."));	

	if (StreamDataSingleton)
	{
		StreamDataSingleton->CanStream = true;

		mWorker = MuxerWorker::JoyInit();

		if (mWorker)
		{
			mWorker->mMuxer->FillAudioBuffer(AudioTracks);
		}
	}

}

// stops stream, releases data
void AGameplayStreamer::StopStream()
{
	UE_LOG(LogTemp, Warning, TEXT("Stop stream..."));

	if (StreamDataSingleton)
	{
		StreamDataSingleton->CanStream = false;
		MuxerWorker::JoyInit()->Stop();
	}
}
// pauses stream, but not realeasing data
void AGameplayStreamer::PauseStream()
{
	UE_LOG(LogTemp, Warning, TEXT("Pause stream..."));

	if (StreamDataSingleton)
	{
		StreamDataSingleton->CanStream = false;
		MuxerWorker::JoyInit()->Stop();
	}
}

void AGameplayStreamer::SetAudioTrack(FString AudioTrackName)
{
	if (mWorker)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, FString::Printf(TEXT("Setting current audio track to : %s"), *AudioTrackName));
		}
		FCriticalSection Locker;
		mWorker->mMuxer->SetAudioTrack(AudioTrackName);
		mWorker->mMuxer->AudioTrackChanged = true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MuxerWorker not initialized yet!!!"));
	}
}

void AGameplayStreamer::SetSilent()
{
	if (mWorker)
	{
		FCriticalSection Locker;
		mWorker->mMuxer->AudioTrackChanged = false;
	}
}

void AGameplayStreamer::FillAudioBuffers(TArray<FString> Tracks)
{
	for (const auto Track : Tracks)
	{
		UE_LOG(LogTemp, Warning, TEXT("Track Name: %s"), *Track);
	}

	if (mWorker)
	{
		FCriticalSection Locker;
		mWorker->mMuxer->FillAudioBuffer(Tracks);
	}
}

void AGameplayStreamer::SetGameSingletonClass()
{
	if (GEngine)
	{
		StreamDataSingleton = (UStreamDataSingleton *)GEngine->GameSingleton;
	}
}

MuxerWorker::MuxerWorker()
{
	UE_LOG(LogTemp, Warning, TEXT("runable ctor"));
	mMuxer = new FFMuxer;
	mThread = FRunnableThread::Create(this, TEXT("MuxerWorker"), 0, TPri_Normal);
}

MuxerWorker::~MuxerWorker()
{
	UE_LOG(LogTemp, Warning, TEXT("runable dtor"));
	delete mThread;
	mThread = nullptr;
	delete mMuxer;
	mMuxer = nullptr;
}

bool MuxerWorker::Init()
{	
	UE_LOG(LogTemp, Warning, TEXT("init runable"));
	mMuxer->Initialize(1280, 720);
	return true;
}

uint32 MuxerWorker::Run()
{
	UE_LOG(LogTemp, Warning, TEXT("Run first call"));

	while (StopTaskCounter.GetValue() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Runing"));
		mMuxer->Mux();
	}
	return uint32();
}

void MuxerWorker::Stop()
{
	UE_LOG(LogTemp, Warning, TEXT("stop runable"));
	StopTaskCounter.Increment();
}

MuxerWorker * MuxerWorker::JoyInit()
{
	UE_LOG(LogTemp, Warning, TEXT("create runable"));
	// Create new instance of thread if it does not exist
	// and the platform supports multi threading!
	if (!Runnable && FPlatformProcess::SupportsMultithreading())
	{
		Runnable = new MuxerWorker();
	}
	return Runnable;
}

void MuxerWorker::Shutdown()
{
	UE_LOG(LogTemp, Warning, TEXT("Shutdown runable"));
	if (Runnable)
	{
		Runnable->EnsureCompletion();
		delete Runnable;
		Runnable = nullptr;
	}
}

void MuxerWorker::EnsureCompletion()
{
	UE_LOG(LogTemp, Warning, TEXT("EnsureCompletion runable"));
	Stop();
	mThread->WaitForCompletion();
}
