// Fill out your copyright notice in the Description page of Project Settings.

#include "StreamGV.h"

void UStreamGV::Draw(FViewport * Viewport, FCanvas * SceneCanvas)
{
	Super::Draw(Viewport, SceneCanvas);

	//initializing Buffer stuff
	FFmpeg::getInstance().init();

	// Adding single frame data to queue
	AddFrameToQueue(Viewport);

	/*TArray<FColor> SingleFrame;
	if (FrameColorQueue.Peek(SingleFrame))
	{
		FFmpeg::getInstance().EncodeFrame(SingleFrame);
	}*/
}

void UStreamGV::BeginDestroy()
{
	Super::BeginDestroy();
	//todo:ashe23 here we realeasing Buffer stuff
	UE_LOG(LogTemp, Warning, TEXT("Gameviewport destroying step!"));
}

void UStreamGV::AddFrameToQueue(FViewport* Viewport)
{
	// Checking if viewport exists
	if (!Viewport)
	{
		UE_LOG(LogTemp, Error, TEXT("No viewport.Aborting"));
		return;
	}

	// Determine viewport size
	FIntPoint ViewportSize = Viewport->GetSizeXY();
	if (ViewportSize.X == 0 || ViewportSize.Y == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No viewport size.Aborting"));
		return;
	}


	// Reading actual pixel data of single frame from viewport
	TArray<FColor> ColorBuffer;
	if (!Viewport->ReadPixels(ColorBuffer, FReadSurfaceDataFlags(),
		FIntRect(0, 0, ViewportSize.X, ViewportSize.Y)))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot read from viewport.Aborting"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Frame %d added to queue"), FrameIndex);
	FrameColorQueue.Enqueue(ColorBuffer);

	// Incrementing frame index
	FrameIndex++;
}
