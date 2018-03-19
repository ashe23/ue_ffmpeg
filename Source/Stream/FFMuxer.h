// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FFHeader.h"
#include "CoreMinimal.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Engine/Public/UnrealClient.h"
#include <string>


struct OutputStream 
{
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
	void Initialize();
	bool IsInitialized() const;
	void Mux(FViewport* Viewport);
	void EndMux();
	void CloseStream(AVFormatContext *oc, OutputStream *ost);
private:
	FViewport * MuxViewport = nullptr;
	bool initialized = false;
	static const int STREAM_DURATION = 10;
	static const int STREAM_FRAME_RATE = 30;
	AVPixelFormat STREAM_PIX_FMT = AVPixelFormat::AV_PIX_FMT_YUV420P;
	const char* SCALE_FLAGS = "SWS_BICUBIC";
	const char* OUTPUT_URL = "C:/screen/test.mp4";
private:
	OutputStream video_st = { 0 };
	OutputStream audio_st = { 0 };
	AVOutputFormat *fmt = nullptr;
	AVFormatContext *oc = nullptr;
	AVCodec *audio_codec = nullptr, *video_codec = nullptr;
	AVDictionary *opt = nullptr;
	int ret;
	int have_video = 0, have_audio = 0;
	int encode_video = 0, encode_audio = 0;
private:
	bool AddStream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec,enum AVCodecID codec_id);
	bool OpenVideo(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
	bool OpenAudio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
	AVFrame* AllocPicture(enum AVPixelFormat pix_fmt, int width, int height);
	AVFrame* AllocAudioFrame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples);
	int WriteVideoFrame(AVFormatContext *oc, OutputStream *ost);
	int WriteAudioFrame(AVFormatContext *oc, OutputStream *ost);
	int WriteFrame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);
	AVFrame* GetVideoFrame(OutputStream *ost);
	AVFrame* GetAudioFrame(OutputStream *ost);

	void FillYUVImage(AVFrame *pict, int frame_index, int width, int height);
	AVRational GetRational(int num, int den);
};
