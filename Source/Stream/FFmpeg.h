// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

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


/**
 * 
 */
class STREAM_API FFmpeg
{
	FFmpeg();
	~FFmpeg();

	void initCodec();
	void StartEncode();
	void FinishEncode();
	void RGB_to_YUV(const TArray<FColor>& ColorBuffer);


	// FFmpeg stuff
	AVFormatContext* ofmt_ctx;
	AVOutputFormat* ofmt;
	AVCodecContext* c;
	AVFrame* frame;
	AVPacket* pkt;
	struct SwsContext *sws_context;
public:
	void EncodeFrame(TArray<FColor> ColorBuffer);
	static FFmpeg& getInstance();
	void init();
	static bool isInitialized;
};
