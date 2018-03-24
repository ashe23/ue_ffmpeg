// Fill out your copyright notice in the Description page of Project Settings.

#include "StreamGV.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"

#define av_err2str(errnum) av_make_error_string((char[AV_ERROR_MAX_STRING_SIZE]){0}, AV_ERROR_MAX_STRING_SIZE, errnum)

void UStreamGV::Draw(FViewport * Viewport, FCanvas * SceneCanvas)
{
	Super::Draw(Viewport, SceneCanvas);
	
	// Printing viewport sizes
	if (GEngine) {
		int32 X = Viewport->GetSizeXY().X;
		int32 Y = Viewport->GetSizeXY().Y;		
	}

	// checking Screen dimensions
	if (!isValidScreenSizes(Viewport))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid Screen size. Must divisible by 2!"));
		return;
	}

	if (!Muxer)
	{
		FIntPoint ViewportSize = Viewport->GetSizeXY();
		Muxer = new FFMuxer;
		Muxer->Initialize(ViewportSize.X,ViewportSize.Y);
	}

	if (Muxer->IsReadyToStream())
	{		
		Muxer->Mux(Viewport);
	}


}

void UStreamGV::BeginDestroy()
{
	Super::BeginDestroy();

	if (Muxer)
	{
		//Muxer->Release();
	}
	//ff_release_resources(); // todo:ashe23 crushes, handle resource managment
	//todo:ashe23 here we realeasing Buffer stuff
	UE_LOG(LogTemp, Warning, TEXT("Gameviewport destroying step!"));
}

// Screen dimensions must be divisible by 2
bool UStreamGV::isValidScreenSizes(FViewport * Viewport)
{
	return Viewport->GetSizeXY().X % 2 == 0 && Viewport->GetSizeXY().Y % 2 == 0;
}