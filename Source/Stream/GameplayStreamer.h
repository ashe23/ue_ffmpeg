// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Async.h"
#include "GameFramework/Actor.h"
#include "Runtime/Core/Public/Templates/UniquePtr.h"
#include "StreamGameMode.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>

#include "GameplayStreamer.generated.h"


class FFMuxer;

class MuxerWorker : public FRunnable
{

public:
	virtual ~MuxerWorker();

public:
	// Begin FRunnable interface.
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	// End FRunnable interface

	/*
	Start the thread and the worker from static (easy access)!
	This code ensures only 1 Prime Number thread will be able to run at a time.
	This function returns a handle to the newly started instance.
	*/
	static MuxerWorker* JoyInit();
	/** Shuts down the thread. Static so it can easily be called from outside the thread context */
	static void Shutdown();

	/** Makes sure this thread has stopped properly */
	void EnsureCompletion();

	FFMuxer* mMuxer = nullptr;
private:
	MuxerWorker();
	static MuxerWorker* Runnable;

private:
	FRunnableThread* mThread = nullptr;
	FThreadSafeCounter StopTaskCounter = 0;
};

UCLASS()
class STREAM_API AGameplayStreamer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGameplayStreamer();
	~AGameplayStreamer();

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

	UFUNCTION(BlueprintCallable, Category = "GameplayStreaming")
	void SetAudioTrack(FString AudioTrackName);

	UFUNCTION(BlueprintCallable, Category = "GameplayStreaming")
	void SetSilent();

	UFUNCTION(BlueprintCallable, Category = "GameplayStreaming")
	void FillAudioBuffers(TArray<FString> AudioBuffers);

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = "GameplayStreaming")
	TArray<FString> AudioTracks;	
private:
	MuxerWorker* mWorker;

	class UStreamDataSingleton* StreamDataSingleton = nullptr;

	void SetGameSingletonClass();
};
