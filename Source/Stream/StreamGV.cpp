// Fill out your copyright notice in the Description page of Project Settings.

#include "StreamGV.h"

#define av_err2str(errnum) av_make_error_string((char[AV_ERROR_MAX_STRING_SIZE]){0}, AV_ERROR_MAX_STRING_SIZE, errnum)

void UStreamGV::Draw(FViewport * Viewport, FCanvas * SceneCanvas)
{
	Super::Draw(Viewport, SceneCanvas);
	
	// Printing viewport sizes
	if (GEngine) {
		int32 X = Viewport->GetSizeXY().X;
		int32 Y = Viewport->GetSizeXY().Y;

		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, FString::Printf(TEXT("Screen Size: %d x %d"), X, Y));
	}

	// checking Screen dimensions
	if (!isValidScreenSizes(Viewport))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid Screen size. Must divisible by 2!"));
		return;
	}

	// initialize FFmpeg stuff
	ff_init(Viewport);


	if (CanStream)
	{
		
		if (StreamOver)
		{
			UE_LOG(LogTemp, Warning, TEXT("Stream over. Releasing data"));
		}
		else
		{
			// Adding single frame data to queue
			//AddFrameToQueue(Viewport);

			UE_LOG(LogTemp, Warning, TEXT("Encoding and writing frames"));
		}		
	}
}

void UStreamGV::BeginDestroy()
{
	Super::BeginDestroy();

	StreamOver = true;
	//todo:ashe23 here we realeasing Buffer stuff
	UE_LOG(LogTemp, Warning, TEXT("Gameviewport destroying step!"));
}

// Screen dimensions must be divisible by 2
bool UStreamGV::isValidScreenSizes(FViewport * Viewport)
{
	return Viewport->GetSizeXY().X % 2 == 0 && Viewport->GetSizeXY().Y % 2 == 0;
}

void UStreamGV::AddFrameToQueue(FViewport* Viewport)
{
	// Checking if viewport exists
	if (!Viewport)
	{
		UE_LOG(LogTemp, Error, TEXT("No viewport.Aborting"));
		return;
	}

	// Determine viewport size
	FIntPoint ViewportSize = Viewport->GetSizeXY();
	if (ViewportSize.X == 0 || ViewportSize.Y == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No viewport size.Aborting"));
		return;
	}


	// Reading actual pixel data of single frame from viewport
	TArray<FColor> ColorBuffer;
	if (!Viewport->ReadPixels(ColorBuffer, FReadSurfaceDataFlags(),
		FIntRect(0, 0, ViewportSize.X, ViewportSize.Y)))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot read from viewport.Aborting"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Frame %d added to queue"), FrameIndex);

	// Incrementing frame index
	FrameIndex++;
}

void UStreamGV::ff_error_log(int ret_err)
{
	char error_buf[256];
	av_make_error_string(error_buf, sizeof(error_buf), ret_err);
	FString ErrDescription{ error_buf };
	UE_LOG(LogTemp, Error, TEXT("Error code: %d"), ret_err);
	UE_LOG(LogTemp, Error, TEXT("Error desc: %s"), *ErrDescription);
}

void UStreamGV::ff_init(FViewport *Viewport)
{
	if (!ff_initialized) {

		UE_LOG(LogTemp, Warning, TEXT("Initializing ffmpeg"));

		av_register_all();
		avformat_network_init();

		ff_init_avformat_context();
		ff_init_io_context();

		// todo:ashe23 need check for nulls
		out_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!out_codec)
		{
			UE_LOG(LogTemp, Error, TEXT("Could not find codec H264!"));
			CanStream = false;
			return;
		}
		// when starting new stream context and codec should already be initialized
		if (ofmt_ctx && out_codec)
		{
			out_stream = avformat_new_stream(ofmt_ctx, out_codec);
			if (!out_stream)
			{
				UE_LOG(LogTemp, Error, TEXT("Could not create new stream!"));
				CanStream = false;
				return;
			}

			out_codec_ctx = avcodec_alloc_context3(out_codec);
			if (!out_codec_ctx)
			{
				UE_LOG(LogTemp, Error, TEXT("Could not alloc context for output!"));
				CanStream = false;
				return;
			}
		}

		ff_set_codec_params(Viewport->GetSizeXY().X, (Viewport->GetSizeXY().Y));
		ff_init_codec_stream();

		out_stream->codecpar->extradata = out_codec_ctx->extradata;
		out_stream->codecpar->extradata_size = out_codec_ctx->extradata_size;

		av_dump_format(ofmt_ctx, 0, output_url.c_str(), 1);

		ff_initialized = true;
	}
}

void UStreamGV::ff_init_avformat_context()
{
	int ret = avformat_alloc_output_context2(&ofmt_ctx, nullptr, "flv", nullptr);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not allocate output format context!"));
		ff_error_log(ret);
		CanStream = false;
	}
}

void UStreamGV::ff_init_io_context()
{
	if (!ofmt_ctx)
	{
		UE_LOG(LogTemp, Error, TEXT("Output context is NULL!"));
		CanStream = false;
		return;
	}

	if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
	{
		int ret = avio_open2(&ofmt_ctx->pb, output_url.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
		if (ret < 0)
		{
			UE_LOG(LogTemp, Error, TEXT("Could not open output IO context!"));
			ff_error_log(ret);
			CanStream = false;
			return;
		}
	}
}

void UStreamGV::ff_set_codec_params(int width, int height)
{
	const int FPS = 30;
	const AVRational dst_fps = { FPS, 1 };

	out_codec_ctx->codec_tag = 0;
	out_codec_ctx->codec_id = AV_CODEC_ID_H264;
	out_codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
	out_codec_ctx->width = width;
	out_codec_ctx->height = height;
	out_codec_ctx->gop_size = 12;
	out_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	out_codec_ctx->framerate = dst_fps;
	out_codec_ctx->time_base = av_inv_q(dst_fps);
	if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
	{
		out_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}
}

void UStreamGV::ff_init_codec_stream()
{
	int ret = avcodec_parameters_from_context(out_stream->codecpar, out_codec_ctx);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not initialize stream codec parameters!"));
		ff_error_log(ret);
		CanStream = false;
		return;		
	}

	AVDictionary *codec_options = nullptr;
	av_dict_set(&codec_options, "profile", "high", 0);
	av_dict_set(&codec_options, "preset", "superfast", 0);
	av_dict_set(&codec_options, "tune", "zerolatency", 0);

	// open video encoder
	ret = avcodec_open2(out_codec_ctx, out_codec, &codec_options);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not open video encoder!"));
		ff_error_log(ret);
		CanStream = false;
		return;		
	}
}
