// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
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
#include <string>
#include <iostream>

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "MyGameViewportClient.generated.h"

/**
 * 
 */
UCLASS()
class STREAM_API UMyGameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()
public:
	UMyGameViewportClient() {
		//FilePath = "C:/screen/test.mp4";
		FilePath = "rtmp://streams4.betconstruct.com/virtualsports/jlp";
		//FilePath = "rtmp://a.rtmp.youtube.com/live2/qx3p-h110-dddb-306x";
		//FilePath = "rtmp://stream-eu1hz.betconstruct.com/virtual_sports/football";
		//FilePath = "rtmp://live.twitch.tv/app/live_44489310_853hMbzjC6MRz3KqaA8NOvD110RfvA";

		UEFilePath = FString{ FilePath.c_str() };
	};

	virtual void Draw(FViewport* Viewport, FCanvas* SceneCanvas) override;

	void InitCodec(FViewport* Viewport);

	void TidyUp();
private:
	UPROPERTY(Config)
	FString DeviceNum;

	UPROPERTY(Config)
	FString H264Crf;

	UPROPERTY(Config)
	int DeviceIndex;

	FIntPoint ViewportSize;
	int count;
	uint32 FrameIndex = 0;

	TArray<FColor> ColorBuffer;
	TArray<uint8> IMG_Buffer;

	struct OutputStream
	{
		AVStream* Stream;
		AVCodecContext* Ctx;

		int64_t NextPts;

		AVFrame* Frame;

		struct SwsContext* SwsCtx;
	};

	OutputStream VideoSt = { 0 };
	AVOutputFormat* Fmt;
	AVFormatContext* FmtCtx;
	AVCodec* VideoCodec;
	AVDictionary* Opt = nullptr;
	SwsContext* SwsCtx;
	AVPacket Pkt;

	int GotOutput;
	int InLineSize[1];

	bool CanStream = false;
	bool Start;
	bool Over;
	bool FirstTime;
	bool Abandon;
	bool AutoRecording;
	bool RecordingNextFrame;
	bool isInit = false;
	double LastSendingTime;


	std::string FilePath;
	FString UEFilePath;
	int32 LevelDelay;

	void EncodeAndWrite();

	void CaptureFrame(FViewport* Viewport);
	void AddStream(enum AVCodecID CodecID);
	void OpenVideo();
	int WriteFrame();
	void CloseStream();
	void AllocPicture();

	int FFmpegEncode(AVFrame *frame);
	
	
};
