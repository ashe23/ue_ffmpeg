// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FFHeader.h"
#include "FFMuxer.h"
#include "FFMediaDataTypes.h"
#include "GameplayStreamer.h"
#include "Engine/GameViewportClient.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Engine/Classes/Engine/Engine.h"
#include "StreamGameMode.h"
#include "StreamGV.generated.h"

DECLARE_STATS_GROUP(TEXT("StreamGV Viewport"), STATGROUP_FFGameplayStreaming, STATCAT_Advanced);

/**
 * 
 */
UCLASS()
class STREAM_API UStreamGV : public UGameViewportClient
{
	GENERATED_BODY()

	void Draw(FViewport* Viewport, FCanvas* SceneCanvas) override;
	void BeginDestroy() override;
	bool isValidScreenSizes(FViewport *Viewport);

private:
	FFMuxer* Muxer = nullptr;
	class UStreamDataSingleton* StreamDataSingleton = nullptr;
	void ReadRGBFromViewportToBuffer(FViewport *Viewport);
	bool SetSingleton();	
};