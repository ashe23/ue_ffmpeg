// Fill out your copyright notice in the Description page of Project Settings.

#include "StreamGV.h"

#include "buffer.h"

void UStreamGV::Draw(FViewport * Viewport, FCanvas * SceneCanvas)
{
	Super::Draw(Viewport, SceneCanvas);
	
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

	ReadRGBFromViewportToBuffer(Viewport);
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
