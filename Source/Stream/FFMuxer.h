// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FFHeader.h"
#include "CoreMinimal.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Engine/Public/UnrealClient.h"
#include <string>
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Core/Public/Misc/Paths.h"

enum class EFrameType
{
	Video,
	Audio
};

// a wrapper around a single output AVStream
struct OutputStream {
	AVStream *st;
	AVCodecContext *enc;

	/* pts of the next frame that will be generated */
	int64_t next_pts;
	int samples_count;

	AVFrame *frame;
	AVFrame *tmp_frame;

	float t, tincr, tincr2;

	struct SwsContext *sws_ctx;
	struct SwrContext *swr_ctx;
};

/**
 * 
 */
class STREAM_API FFMuxer
{
public:	
	~FFMuxer();
	void Initialize(int32 Width,int32 Height);
	bool IsInitialized() const;
	bool IsReadyToStream() const;
	void Mux(FViewport* Viewport);
	void Release();	
	void AddVideoStream(enum AVCodecID codec_id);
	void AddAudioStream(enum AVCodecID codec_id);
private:
	bool initialized = false;
	bool CanStream = false;
	int width;
	int height;
	const char *filename = "C:/screen/test.mp4";
	/// remove later
	OutputStream *video_st, *audio_st;
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVCodec *audio_codec, *video_codec;
	int ret;
	int have_video = 0, have_audio = 0;
	int encode_video = 0, encode_audio = 0;
	AVDictionary *opt = NULL;
	int i;
};
