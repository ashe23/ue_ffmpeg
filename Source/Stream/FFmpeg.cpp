// Fill out your copyright notice in the Description page of Project Settings.

#include "FFmpeg.h"
bool FFmpeg::isInitialized = false;

FFmpeg::FFmpeg()
{
}

FFmpeg::~FFmpeg()
{
}

void FFmpeg::initCodec()
{
	avcodec_register_all();
	av_register_all();
	avformat_network_init();
	av_log_set_level(AV_LOG_WARNING);

	// set codec and context
	StartEncode();
	//int pts;
	//for (pts = 0; pts < 10000; pts++) {
	//	frame->pts = pts;
	//	//rgb = generate_rgb(width, height, pts, rgb);
	//	//ffmpeg_encoder_encode_frame(rgb);
	//}
	//FinishEncode();
	
}

void FFmpeg::StartEncode()
{
	AVCodec *codec;
	int ret;
	codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec)
	{
		UE_LOG(LogTemp, Error, TEXT("FFmpeg: cant find codec"));
		return;
	}
	c = avcodec_alloc_context3(codec);
	if (!c)
	{
		UE_LOG(LogTemp, Error, TEXT("FFmpeg: Could not allocate video codec context"));
		return;
	}
	c->bit_rate = 400000;
	c->width = 640;
	c->height = 320;
	c->time_base.num = 1;
	c->time_base.den = 30;
	c->gop_size = 10;
	c->max_b_frames = 1;
	c->pix_fmt = AV_PIX_FMT_YUV420P;

	av_opt_set(c->priv_data, "preset", "slow", 0);

	if (avcodec_open2(c, codec, NULL) < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FFmpeg: Could not open codec"));
		return;
	}

	frame = av_frame_alloc();
	if (!frame)
	{
		UE_LOG(LogTemp, Error, TEXT("FFmpeg: Could not allocate video frame"));
		return;
	}
	frame->format = c->pix_fmt;
	frame->width = c->width;
	frame->height = c->height;
	ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height, c->pix_fmt, 32);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FFmpeg: Could not allocate raw picture buffer"));
		return;
	}
}

void FFmpeg::FinishEncode()
{
	//uint8_t endcode[] = { 0, 0, 1, 0xb7 };
	//int got_output, ret;
	//do {
	//	fflush(stdout);
	//	//ret = avcodec_encode_video2(c, pkt, NULL, &got_output);// error
	//	if (ret < 0) {
	//		UE_LOG(LogTemp, Error, TEXT("FFmpeg: Error encoding frame"));
	//		return;
	//	}
	//	if (got_output) {
	//		av_packet_unref(pkt);
	//	}
	//} while (got_output);

	//avcodec_close(c);
	//av_free(c);
	//av_freep(&frame->data[0]);
	//av_frame_free(&frame);
}

void FFmpeg::RGB_to_YUV(const TArray<FColor>& ColorBuffer)
{
	TArray<uint8> IMG_Buffer;
	uint8* inData[1] = { IMG_Buffer.GetData() };

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

	const int in_linesize[1] = { 3 * c->width };
	sws_context = sws_getCachedContext(sws_context,
		c->width, c->height, AV_PIX_FMT_RGB24,
		c->width, c->height, AV_PIX_FMT_YUV420P,
		0, 0, 0, 0);
	sws_scale(sws_context, (const uint8_t * const *)&DestPtr, in_linesize, 0,
		c->height, frame->data, frame->linesize);
}

void FFmpeg::EncodeFrame(TArray<FColor> ColorBuffer)
{
	int ret, got_output;
	// converting RGB => YUV
	RGB_to_YUV(ColorBuffer);

	// creating packet
	av_init_packet(pkt);
	pkt->data = nullptr;
	pkt->size = 0;
	ret = avcodec_encode_video2(c, pkt, frame, &got_output);
	if (ret < 0) {
		UE_LOG(LogTemp, Error, TEXT("FFmpeg: Error encoding frame"));
		exit(1);
	}
	if (got_output) {
		av_packet_unref(pkt);
	}
}

FFmpeg & FFmpeg::getInstance()
{
	static FFmpeg instance;
	return instance;
}

void FFmpeg::init()
{
	if (!isInitialized) 
	{
		UE_LOG(LogTemp, Warning, TEXT("FFmpeg: initialization"));
		isInitialized = true;		
		initCodec();
		//todo:ashe23 Creating new thread that reads one by one FFmpeg frames and passes it to outside using FFmpeg
	}
}


