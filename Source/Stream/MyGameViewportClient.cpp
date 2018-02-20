// Fill out your copyright notice in the Description page of Project Settings.

#include "MyGameViewportClient.h"

#define FRAMERATE 30

void UMyGameViewportClient::Draw(FViewport* Viewport, FCanvas* SceneCanvas)
{
	Super::Draw(Viewport, SceneCanvas);

	InitCodec(Viewport);

	if (CanStream) {
		if (Over)  // You may need to set this in other class
		{
			Over = false;
			TidyUp();
		}

		else {
			CaptureFrame(Viewport);
		}
	}
}

void UMyGameViewportClient::InitCodec(FViewport* Viewport)
{
	if (!isInit)
	{
		ViewportSize = Viewport->GetSizeXY();

		av_register_all();
		avformat_network_init();

		av_log_set_level(AV_LOG_DEBUG);

		avformat_alloc_output_context2(&FmtCtx, nullptr, "flv", FilePath.c_str());
		if (!FmtCtx)
		{
			UE_LOG(LogTemp, Error, TEXT("cannot alloc format context"));
			return;
		}
		Fmt = FmtCtx->oformat;

		auto codec_id = AV_CODEC_ID_H264;
		auto codec = avcodec_find_encoder(codec_id);
		//const char codec_name[32] = "h264_nvenc";
		//const char codec_name[32] = "libx264";
		//auto codec = avcodec_find_encoder_by_name(codec_name);
		if (!codec)
		{
			//UE_LOG(LogTemp, Error, TEXT("Cant Find Codec: %s"), codec_name);
			return;			
		}

		av_format_set_video_codec(FmtCtx, codec);

		if (Fmt->video_codec != AV_CODEC_ID_NONE)
		{
			AddStream(codec_id);
		}
		OpenVideo();
		VideoSt.NextPts = 0;

		// Dump Format of output context
		av_dump_format(FmtCtx, 0, FilePath.c_str(), 1);

		if (!(Fmt->flags & AVFMT_NOFILE))
		{
			auto ret = avio_open(&FmtCtx->pb, FilePath.c_str(), AVIO_FLAG_WRITE);
			if (ret < 0)
			{
				UE_LOG(LogTemp, Error, TEXT("Could not open %s: %s"), *UEFilePath);
				return;
			}
		}

		auto ret = avformat_write_header(FmtCtx, &Opt);
		if (ret < 0)
		{
			UE_LOG(LogTemp, Error, TEXT("Error occurred when writing header to: %s"), *UEFilePath);
			return;
		}

		InLineSize[0] = 4 * VideoSt.Ctx->width;
		SwsCtx = sws_getContext(VideoSt.Ctx->width, VideoSt.Ctx->height, AV_PIX_FMT_RGBA,
			VideoSt.Ctx->width, VideoSt.Ctx->height, VideoSt.Ctx->pix_fmt,
			0, nullptr, nullptr, nullptr);


		UE_LOG(LogTemp, Warning, TEXT("Codec initialized successfully"));
		isInit = true;
		CanStream = true;
	}
}

void UMyGameViewportClient::OpenVideo()
{
	auto c = VideoSt.Ctx;
	AVDictionary* opt = nullptr;

	av_dict_copy(&opt, Opt, 0);

	auto ret = avcodec_open2(c, VideoCodec, &opt);
	av_dict_free(&opt);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not open video codec"));
	}

	AllocPicture();
	if (!VideoSt.Frame)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not allocate video frame"));
		return;
	}
	if (avcodec_parameters_from_context(VideoSt.Stream->codecpar, c))
	{
		UE_LOG(LogTemp, Error, TEXT("Could not copy the stream parameters"));
	}
}

void UMyGameViewportClient::AllocPicture()
{
	VideoSt.Frame = av_frame_alloc();
	if (!VideoSt.Frame)
	{
		UE_LOG(LogTemp, Error, TEXT("av_frame_alloc failed."));
		return;
	}

	VideoSt.Frame->format = VideoSt.Ctx->pix_fmt;
	VideoSt.Frame->width = ViewportSize.X;
	VideoSt.Frame->height = ViewportSize.Y;

	if (av_frame_get_buffer(VideoSt.Frame, 32) < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not allocate frame data"));
	}
}

void UMyGameViewportClient::AddStream(enum AVCodecID CodecID)
{
	VideoCodec = avcodec_find_encoder(CodecID);
	if (!VideoCodec)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find encoder for '%s'"), ANSI_TO_TCHAR(avcodec_get_name(CodecID)));
	}


	VideoSt.Stream = avformat_new_stream(FmtCtx, VideoCodec);
	if (!VideoSt.Stream)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not allocate stream"));
	}

	VideoSt.Stream->id = FmtCtx->nb_streams - 1;
	VideoSt.Ctx = VideoSt.Stream->codec;
	VideoSt.Ctx = avcodec_alloc_context3(VideoCodec);
	if (!VideoSt.Ctx)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not alloc an encoding context"));
	}


	// if codec type is video set h264 settings
	if (VideoSt.Ctx->codec->type == AVMEDIA_TYPE_VIDEO) {
		AVRational timeBase{ 1, FRAMERATE };
		VideoSt.Ctx->codec_id = CodecID;
		VideoSt.Ctx->width = ViewportSize.X;
		VideoSt.Ctx->height = ViewportSize.Y;
		VideoSt.Stream->time_base = VideoSt.Ctx->time_base = { 1, FRAMERATE };
		VideoSt.Ctx->time_base = timeBase;
		VideoSt.Ctx->gop_size = 12;
		VideoSt.Ctx->bit_rate = 400000;
		//VideoSt.Ctx->max_b_frames = 1;
		VideoSt.Ctx->pix_fmt = AV_PIX_FMT_YUV420P;
		VideoSt.Ctx->qmin = 2;
		VideoSt.Ctx->qmax = 31;
		VideoSt.Ctx->max_qdiff = 3;
		VideoSt.Ctx->qcompress = 0.5f;

		av_opt_set(VideoSt.Ctx, "tune", "film", 0);
		av_opt_set(VideoSt.Ctx, "preset", "veryslow", 0);

	}


	//av_opt_set(VideoSt.Ctx->priv_data, "cq", TCHAR_TO_ANSI(*H264Crf), 0);  // change `cq` to `crf` if using libx264
	//av_opt_set(VideoSt.Ctx->priv_data, "gpu", TCHAR_TO_ANSI(*DeviceNum), 0); // comment this line if using libx264

	if (FmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
	{
		VideoSt.Ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}
}

void UMyGameViewportClient::EncodeAndWrite()
{
	Pkt = { nullptr };
	av_init_packet(&Pkt);

	fflush(stdout);

	IMG_Buffer.SetNum(ColorBuffer.Num() * 4);
	uint8* DestPtr = nullptr;
	for (auto i = 0; i < ColorBuffer.Num(); i++)
	{
		DestPtr = &IMG_Buffer[i * 4];
		auto SrcPtr = ColorBuffer[i];
		*DestPtr++ = SrcPtr.R;
		*DestPtr++ = SrcPtr.G;
		*DestPtr++ = SrcPtr.B;
		*DestPtr++ = SrcPtr.A;
	}

	uint8* inData[1] = { IMG_Buffer.GetData() };
	sws_scale(SwsCtx, inData, InLineSize, 0, VideoSt.Ctx->height, VideoSt.Frame->data, VideoSt.Frame->linesize);

	VideoSt.Frame->pts = VideoSt.NextPts++;
	if (FFmpegEncode(VideoSt.Frame) < 0)
		UE_LOG(LogTemp, Error, TEXT("Error encoding frame %d"), count);

	auto ret = WriteFrame();
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Error while writing video frame"));
	}
	av_packet_unref(&Pkt);
}

int UMyGameViewportClient::WriteFrame()
{
	UE_LOG(LogTemp, Warning, TEXT("Encoding Frame : %d"), FrameIndex);
	av_packet_rescale_ts(&Pkt, VideoSt.Ctx->time_base, VideoSt.Stream->time_base);
	Pkt.stream_index = VideoSt.Stream->index;
	FrameIndex++;
	return av_interleaved_write_frame(FmtCtx, &Pkt);
}

int UMyGameViewportClient::FFmpegEncode(AVFrame *frame) {
	GotOutput = 0;
	auto ret = avcodec_send_frame(VideoSt.Ctx, frame);
	if (ret < 0 && ret != AVERROR_EOF) {
		UE_LOG(LogTemp, Warning, TEXT("error during sending frame, error"));
		return -1;
	}

	ret = avcodec_receive_packet(VideoSt.Ctx, &Pkt);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		return 0;

	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Error during receiving frame, error "));
		av_packet_unref(&Pkt);
		return -1;
	}

	GotOutput = 1;
	return 0;
}

void UMyGameViewportClient::CloseStream()
{
	avcodec_free_context(&VideoSt.Ctx);
	av_frame_free(&VideoSt.Frame);
	sws_freeContext(SwsCtx);

	if (!(Fmt->flags & AVFMT_NOFILE))
	{
		auto ret = avio_closep(&FmtCtx->pb);
		if (ret < 0)
		{			
			UE_LOG(LogTemp, Error, TEXT("avio close failed"));
		}
	}

	avformat_free_context(FmtCtx);
}

void UMyGameViewportClient::TidyUp()
{
	/* get the delayed frames */
	for (GotOutput = 1; GotOutput; count++)
	{
		fflush(stdout);

		FFmpegEncode(nullptr);

		if (GotOutput)
		{
			auto ret = WriteFrame();
			if (ret < 0)
			{
				UE_LOG(LogTemp, Error, TEXT("Error while writing video frame"));
			}
			av_packet_unref(&Pkt);
		}
	}

	auto ret = av_write_trailer(FmtCtx);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("writing trailer error"));
	}

	CloseStream();
}



void UMyGameViewportClient::CaptureFrame(FViewport* Viewport)
{
	if (!Viewport) {
		UE_LOG(LogTemp, Error, TEXT("No viewport"));
		return;
	}

	if (ViewportSize.X == 0 || ViewportSize.Y == 0) {
		UE_LOG(LogTemp, Error, TEXT("Viewport size is 0"));
		return;
	}

	ColorBuffer.Empty();

	if (!Viewport->ReadPixels(ColorBuffer, FReadSurfaceDataFlags(),
		FIntRect(0, 0, ViewportSize.X, ViewportSize.Y)))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot read from viewport"));
		return;
	}

	EncodeAndWrite();  // call InitCodec() before this
}

