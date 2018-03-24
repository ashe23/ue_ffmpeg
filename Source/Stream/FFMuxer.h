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
	AVStream *st = nullptr;
	AVCodecContext *enc = nullptr;

	/* pts of the next frame that will be generated */
	int64_t next_pts = 0;
	int samples_count = 0;

	AVFrame *frame = nullptr;
	AVFrame *tmp_frame = nullptr;

	float t= 0, tincr=0, tincr2=0;

	SwsContext *sws_ctx = nullptr;
	SwrContext *swr_ctx = nullptr;

	uint8* frame_buf = nullptr;
	int audio_buffer_size = 0;
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
	void PrintEngineError(FString ErrorString);
	void PrintEngineWarning(FString Text);
private:
	void AddVideoStream();
	void AddAudioStream();
	void OpenVideo();
	void OpenAudio();
	AVFrame* AllocPicture(enum AVPixelFormat pix_fmt, int w, int h);
	AVFrame* AllocAudioFrame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples);
	int WriteVideoFrame(FViewport * Viewport);
	int WriteAudioFrame();
	int WriteFrame(const AVRational *time_base, AVStream *st, AVPacket *pkt);

	AVFrame* GetVideoFrame(FViewport * Viewport);
	AVFrame* GetAudioFrame();

	void CloseVideoStream();
	void CloseAudioStream();

	void FillYUVImage(FViewport * Viewport, AVFrame* Frame);
private:
	bool initialized = false;
	bool CanStream = false;
	bool MuxingLoopStarted = false;
	int width;
	int height;
	const char *filename = "C:/screen/test.mp4";
	FString AudioFileName = "input.pcm";
	TArray<uint8> AudioFileBuffer;
	TArray<uint16> Buf;

	/// remove later
	AVFormatContext *FormatContext = nullptr;
	AVOutputFormat* OutputFormat = nullptr;
	AVDictionary *Dictionary = nullptr;
	AVCodec* VideoCodec = nullptr;
	AVCodec* AudioCodec = nullptr;
	int ret;
	int have_video = 0, have_audio = 0;
	int encode_video = 0, encode_audio = 0;

	OutputStream audio_st;
	OutputStream video_st;
};
