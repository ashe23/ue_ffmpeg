// Fill out your copyright notice in the Description page of Project Settings.

#include "AudioRecorder.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/HAL/PlatformFilemanager.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Engine/Classes/Components/AudioComponent.h"
#include "Runtime/Engine/Public/AudioDecompress.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundCue.h"
#include "AudioDevice.h"
#include "ConstructorHelpers.h"
#include "Engine.h"
#include "WaveFileManager.h"
#include "Runtime/Engine/Public/AudioDeviceManager.h"

//#define av_err2str(errnum) av_make_error_string((char[AV_ERROR_MAX_STRING_SIZE]){0}, AV_ERROR_MAX_STRING_SIZE, errnum)

// Sets default values
AAudioRecorder::AAudioRecorder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAudioRecorder::BeginPlay()
{
	Super::BeginPlay();
	
	if (SoundWaves.Num() > 0)
	{
		// For every sound wave creating unique audio component
		for (auto const &SoundWave : SoundWaves)
		{
			FString CueName = SoundWave->GetName();
			FName AudioCompName(*CueName);

			// creating audio component
			auto NewAudioComponent = NewObject<UAudioComponent>(this, AudioCompName);
			if (NewAudioComponent)
			{
				// Attaching Sound Wave to AudioComponent
				NewAudioComponent->SetSound(SoundWave);
				
				//// Registering Delegates
				//NewAudioComponent->OnAudioPlaybackPercent.AddDynamic(this, &AAudioRecorder::AudioPlayPercent);
				//NewAudioComponent->OnAudioFinished.AddDynamic(this, &AAudioRecorder::AudioFinished);

				//// Adding to NewAudioComponent to Array list
				//UE_LOG(LogTemp, Warning, TEXT("Audio component : %s successfully created"), *CueName);
				//AudioComponents.Add(NewAudioComponent);

				//// Decoding SoundWave
				//DecodeSoundWave(SoundWave);
				
				// Playing
				NewAudioComponent->Play();

				//AddNewEvent(SoundWave, 0.2, 0.7);
			}
		}
	}
}

void AAudioRecorder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogTemp, Warning, TEXT("EventBuffer size: %d"), EventsBuffer.Num());
	// When game ends we must parse all Soundbuffer and fill in main audio buffer
	

	

	if (SoundWaves.IsValidIndex(0))
	{
		USoundWave* TempWave = SoundWaves[0];
		if (TempWave)
		{
			if (TempWave->RawPCMData == nullptr)
			{
				DecodeSoundWave(TempWave);
			}
			// todo:ashe23 extract as method

			//int32 outputSize = 0.2 * TempWave->RawPCMDataSize / TempWave->Duration;
			MainAudioBuffer.Append(TempWave->RawPCMData, TempWave->RawPCMDataSize);
			UE_LOG(LogTemp, Warning, TEXT("Original Size: %d"), TempWave->RawPCMDataSize);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Cant find decoded PCM data"));
		}		
	}

	// mixing 2 buffers of ding


	// saving file as .wav
	WaveFileManager WFManager;
	if (MainAudioBuffer.Num() > 0)
	{
		uint8* Data = (uint8*)MainAudioBuffer.GetData();
		TArray<uint8> WaveFileData;
		WFManager.Serialize(WaveFileData, Data, MainAudioBuffer.Num());
	}
}

void AAudioRecorder::DecodeAudioFile()
{
	// 1) Grab Raw Audio and Video Datas
	// 2) Mux them together
	// 3) Stream over RTMP or save to file


	// Raw PCM data of audio file
	FString InputFileURL = FPaths::ProjectDir() + "ThirdParty/audio/input.pcm";
	FString OutputFileURL = FPaths::ProjectDir() + "ThirdParty/audio/output.aac";
	
}

// Called every frame
void AAudioRecorder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAudioRecorder::DecodeSoundWave(USoundWave * PlayingSoundWave)
{
	if (PlayingSoundWave->RawPCMData == NULL)
	{
		UE_LOG(LogTemp, Warning, TEXT("Sound Wave: '%s' decoded successfully."), *PlayingSoundWave->GetName());
		FAudioDevice* AudioDevice = GEngine->GetMainAudioDevice();
		if (AudioDevice)
		{
			EDecompressionType DecompressionType = PlayingSoundWave->DecompressionType;
			PlayingSoundWave->DecompressionType = DTYPE_Native;

			FName AudioResourceFormat = AudioDevice->GetRuntimeFormat(PlayingSoundWave);
			if (
				PlayingSoundWave->InitAudioResource(AudioResourceFormat) &&
				(PlayingSoundWave->DecompressionType != DTYPE_RealTime || PlayingSoundWave->CachedRealtimeFirstBuffer == nullptr)
				)
			{
				FAsyncAudioDecompress TempDecompressor(PlayingSoundWave);
				TempDecompressor.StartSynchronousTask();
			}

			PlayingSoundWave->DecompressionType = DecompressionType;

		}
	}
}

void AAudioRecorder::ff_error_log(int ret_err)
{
	char error_buf[256];
	av_make_error_string(error_buf, sizeof(error_buf), ret_err);
	FString ErrDescription{ error_buf };
	UE_LOG(LogTemp, Error, TEXT("Error code: %d"), ret_err);
	UE_LOG(LogTemp, Error, TEXT("Error desc: %s"), *ErrDescription);
}


void AAudioRecorder::AudioPlayPercent(const USoundWave * PlayingSoundWave, const float PlayBackPercent)
{

	float CurrentTime = UGameplayStatics::GetRealTimeSeconds(GetWorld());
	UE_LOG(LogTemp, Warning, TEXT("Current percent: %f, Time: %f"), PlayBackPercent * 100, CurrentTime);
}

void AAudioRecorder::AudioFinished()
{
	float EndTime = UGameplayStatics::GetRealTimeSeconds(GetWorld());
	UE_LOG(LogTemp, Warning, TEXT("Audio Finished playing , Time: %f"), EndTime);
}

// Adding new event to sound event buffer
void AAudioRecorder::AddNewEvent(USoundWave * SoundWave, float StartTime, float EndTime)
{
	if (SoundWave)
	{
		UE_LOG(LogTemp, Warning, TEXT("Added new Sound event to buffer"));
		FSoundBufferEvent NewEvent{ SoundWave, StartTime, EndTime };

		//EventsBuffer.Add(NewEvent);
	}
}
