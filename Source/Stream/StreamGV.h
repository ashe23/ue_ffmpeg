// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "Runtime/Core/Public/Containers/Queue.h"
#include "FFmpeg.h"
#include "StreamGV.generated.h"

/**
 * 
 */
UCLASS()
class STREAM_API UStreamGV : public UGameViewportClient
{
	GENERATED_BODY()
	
	
	void Draw(FViewport* Viewport, FCanvas* SceneCanvas) override;
	void BeginDestroy() override;

	// Adds single frame rgba data to queue
	void AddFrameToQueue(FViewport* Viewport);

	// Holds rgba data of frames
	TQueue<TArray<FColor>> FrameColorQueue;
	int32 FrameIndex = 0;	
	
};
