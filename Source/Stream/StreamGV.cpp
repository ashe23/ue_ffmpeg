// Fill out your copyright notice in the Description page of Project Settings.

#include "StreamGV.h"

void UStreamGV::Draw(FViewport * Viewport, FCanvas * SceneCanvas)
{
	Super::Draw(Viewport, SceneCanvas);
	
	// checking Screen dimensions
	if (!isValidScreenSizes(Viewport))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid Screen size. Must divisible by 2!"));
		return;
	}

	ReadRGBFromViewportToBuffer(Viewport);
	
	/*if (!Muxer)
	{
		FIntPoint ViewportSize = Viewport->GetSizeXY();
		Muxer = new FFMuxer;
		Muxer->Initialize(ViewportSize.X,ViewportSize.Y);
	}

	if (Muxer->IsReadyToStream())
	{		
		Muxer->Mux(Viewport);
	}*/


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

void UStreamGV::ReadRGBFromViewportToBuffer(FViewport * Viewport)
{
	if (!Viewport)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid viewport. Aborting!"));
		return;
	}

	FIntPoint ViewportSize = Viewport->GetSizeXY();

	UE_LOG(LogTemp,Warning,TEXT("HELLO from render thread"));
	// Reading actual pixel data of single frame from viewport
	TArray<FColor> ColorBuffer;
	TArray<uint8> SingleFrameBuffer;

	if (!Viewport->ReadPixels(ColorBuffer, FReadSurfaceDataFlags(), FIntRect(0, 0, ViewportSize.X, ViewportSize.Y)))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot read from viewport. Aborting!"));
		return;
	}

	// converting from TArray to const uint8*
	SingleFrameBuffer.Empty();
	SingleFrameBuffer.SetNum(ColorBuffer.Num() * 4);
	uint8* DestPtr = nullptr;
	for (auto i = 0; i < ColorBuffer.Num(); i++)
	{
		DestPtr = &SingleFrameBuffer[i * 4];
		auto SrcPtr = ColorBuffer[i];
		*DestPtr++ = SrcPtr.R;
		*DestPtr++ = SrcPtr.G;
		*DestPtr++ = SrcPtr.B;
		*DestPtr++ = SrcPtr.A;
	}

	// filling frame with actual data
	VideoFrame Frame; // todo:ashe23 maybe optimize this
	Frame.Data = SingleFrameBuffer;

	UE_LOG(LogTemp, Warning, TEXT("Frame Size:%d"), Frame.Data.Num());
}
