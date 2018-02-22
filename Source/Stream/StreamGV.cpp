// Fill out your copyright notice in the Description page of Project Settings.

#include "StreamGV.h"

#define av_err2str(errnum) av_make_error_string((char[AV_ERROR_MAX_STRING_SIZE]){0}, AV_ERROR_MAX_STRING_SIZE, errnum)

void UStreamGV::Draw(FViewport * Viewport, FCanvas * SceneCanvas)
{
	Super::Draw(Viewport, SceneCanvas);

	// checking Screen dimensions
	if (!isValidScreenSizes(Viewport))
	{
		return;
	}

	// initialize FFmpeg stuff


	// Adding single frame data to queue
	AddFrameToQueue(Viewport);

	if (CanStream)
	{
		if (StreamOver)
		{
			UE_LOG(LogTemp, Warning, TEXT("Stream over. Releasing data"));
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("Encoding and writing frames"));
		}
		// if stream over
			// release resources
		// else
			// encode frame
			// write frame
	}
}

void UStreamGV::BeginDestroy()
{
	Super::BeginDestroy();

	StreamOver = true;
	//todo:ashe23 here we realeasing Buffer stuff
	UE_LOG(LogTemp, Warning, TEXT("Gameviewport destroying step!"));
}

// Screen dimensions must be divisible by 2
bool UStreamGV::isValidScreenSizes(FViewport * Viewport)
{
	return Viewport->GetSizeXY().X % 2 == 0 && Viewport->GetSizeXY().Y % 2 == 0;
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

	// Incrementing frame index
	FrameIndex++;
}

void UStreamGV::ff_error_log(int ret_err)
{
	char error_buf[256];
	av_make_error_string(error_buf, sizeof(error_buf), ret_err);
	FString ErrDescription{ error_buf };
	UE_LOG(LogTemp, Error, TEXT("Error code: %d"), ret_err);
	UE_LOG(LogTemp, Error, TEXT("Error desc: %s"), *ErrDescription);
}
