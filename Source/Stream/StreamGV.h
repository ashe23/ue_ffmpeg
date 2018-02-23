// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <string>
extern "C"
{
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavutil/error.h"
#include "libavutil/time.h"
}

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "Runtime/Core/Public/Containers/Queue.h"
#include "Runtime/Engine/Classes/Engine/Engine.h"
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
	bool isValidScreenSizes(FViewport *Viewport);
		

	// Adds single frame rgba data to queue
	void AddFrameToQueue(FViewport* Viewport);
	
	int32 FrameIndex = 0;	
	
	bool CanStream = true;
	bool StreamOver = false;

	// hold RGB data about single frame
	//TArray<FColor> ColorBuffer;
	TArray<uint8> SingleFrameBuffer;

	//std::string output_url = "C:/screen/test.flv";

	std::string output_url = "rtmp://live.twitch.tv/app/live_44489310_853hMbzjC6MRz3KqaA8NOvD110RfvA";
	
	// FFMPEG stuff
	AVFormatContext* ofmt_ctx = nullptr;
	AVCodec* out_codec = nullptr;
	AVStream* out_stream = nullptr;
	AVCodecContext* out_codec_ctx = nullptr;
	SwsContext* swsctx = nullptr;
	AVFrame* frame = nullptr;
	bool ff_initialized = false;


	void ff_error_log(int ret_err);
	// main init func , only this must be called
	void ff_init(FViewport *Viewport);
	void ff_init_avformat_context();
	void ff_init_io_context();
	void ff_init_codec_stream();
	void ff_set_codec_params(int width, int height);
	void ff_init_sample_scaler(int width, int height);
	void ff_alloc_frame_buffer(int width, int height);
	void ff_write_frame();
	void ff_encode_and_write_frame(FViewport* Viewport);
	void ff_release_resources();
};
