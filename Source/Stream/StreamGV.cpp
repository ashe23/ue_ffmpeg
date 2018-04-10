// Fill out your copyright notice in the Description page of Project Settings.

#include "StreamGV.h"
#include "Engine.h"
#include "buffer.h"
#include "StreamDataSingleton.h"

DECLARE_CYCLE_STAT(TEXT("ReadPixels"), STAT_ReadPixels, STATGROUP_FFGameplayStreaming);
DECLARE_CYCLE_STAT(TEXT("StreamGV Draw function"), STAT_StreamGVDraw, STATGROUP_FFGameplayStreaming);

void UStreamGV::Draw(FViewport * Viewport, FCanvas * SceneCanvas)
{
	Super::Draw(Viewport, SceneCanvas);
	
	SCOPE_CYCLE_COUNTER(STAT_StreamGVDraw);	

	if (!SetSingleton())
	{
		UE_LOG(LogTemp, Error, TEXT("Game mode not setted"));
		return;
	}

	if (!Viewport)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid Viewport!"));
		return;
	}

	if (!isValidScreenSizes(Viewport))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid Screen size. Must divisible by 2!"));
		return;
	}

	check(StreamDataSingleton);

	UE_LOG(LogTemp, Warning, TEXT("CanStream in singleton : %s"), StreamDataSingleton->CanStream ? TEXT("True") : TEXT("False"));

	if (StreamDataSingleton->CanStream)
	{
		ReadRGBFromViewportToBuffer(Viewport);
	}
}

void UStreamGV::BeginDestroy()
{
	Super::BeginDestroy();
	
	UE_LOG(LogTemp, Warning, TEXT("Gameviewport destroying step!"));
}

// Screen dimensions must be divisible by 2
bool UStreamGV::isValidScreenSizes(FViewport * Viewport)
{
	return Viewport->GetSizeXY().X % 2 == 0 && Viewport->GetSizeXY().Y % 2 == 0;
}

void UStreamGV::ReadRGBFromViewportToBuffer(FViewport * Viewport)
{
	// TODO:ashe23 bottle neck in read pixels, optimize later
	SCOPE_CYCLE_COUNTER(STAT_ReadPixels);

	auto ViewportSize = Viewport->GetSizeXY();
	TArray<FColor> ColorBuffer;
	ColorBuffer.Reserve(Viewport->GetSizeXY().X * Viewport->GetSizeXY().Y);


	if (!Viewport->ReadPixels(ColorBuffer, FReadSurfaceDataFlags(),	FIntRect(0, 0, ViewportSize.X, ViewportSize.Y)))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot read from viewport.Aborting"));
		return;
	}

	VideoBuffer::GetInstance().add(ColorBuffer);
}

// Sets game mode if its not exists
bool UStreamGV::SetSingleton()
{
	if (!StreamDataSingleton)
	{
		if (GEngine)
		{
			StreamDataSingleton = (UStreamDataSingleton*)GEngine->GameSingleton;
			if (StreamDataSingleton)
			{
				return true;
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("StreamDataSingleton not found"));
				return false;
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("GEngine is null"));
			return false;
		}
	}


	return true;
}
