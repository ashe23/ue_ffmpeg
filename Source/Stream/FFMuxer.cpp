// Fill out your copyright notice in the Description page of Project Settings.

#include "FFMuxer.h"

void FFMuxer::Initialize()
{
	if (!initialized)
	{
		UE_LOG(LogTemp,Warning,TEXT("Initializing ffmpeg"));
		
		InitFFmpeg();

		if (InitOutputFormatContext() && InitIOContext())
		{
			if (InitCodecs())
			{
				if (InitStreams())
				{
					if (OpenCodecs())
					{
						av_dump_format(OutputFormatContext, 0, OUTPUT_URL, 1);

						// Writing header
						if (WriteHeader())
						{
							UE_LOG(LogTemp, Warning, TEXT("Initialized Successfully"));
							CanStream = true;
						}						
					}

				}

			}
		}

		if (!CanStream)
		{
			UE_LOG(LogTemp, Error, TEXT("Initialization Failed"));
		}


		initialized = true;
	}
}

bool FFMuxer::IsInitialized() const
{
	return initialized;
}

bool FFMuxer::IsReadyToStream() const
{
	return CanStream;
}

void FFMuxer::Mux(FViewport* Viewport)
{
	if (CanStream)
	{
		UE_LOG(LogTemp, Warning, TEXT("Muxing"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Errors occured when initializing ffmpeg stuff"));
	}
}

void FFMuxer::Release()
{
	avformat_free_context(OutputFormatContext);
}


void FFMuxer::InitFFmpeg()
{
	av_register_all();
	avformat_network_init();
}

bool FFMuxer::InitOutputFormatContext()
{
	int ret = avformat_alloc_output_context2(&OutputFormatContext, nullptr, nullptr, OUTPUT_URL);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not allocate output format context!"));
		PrintError(ret);
		return false;
	}
	OutputFormat = OutputFormatContext->oformat;

	return true;
}

bool FFMuxer::InitIOContext()
{
	if (!(OutputFormatContext->oformat->flags & AVFMT_NOFILE))
	{
		int ret = avio_open2(&OutputFormatContext->pb, OUTPUT_URL, AVIO_FLAG_WRITE, nullptr, nullptr);
		if (ret < 0)
		{
			UE_LOG(LogTemp, Error, TEXT("Could not open output IO context!"));
			return false;
		}
	}
	return true;
}

bool FFMuxer::InitCodecs()
{
	// setting audio codec
	AudioCodec = avcodec_find_encoder(AV_CODEC_ID_MP3);
	if (!AudioCodec)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find 'MP3' codec!"));
		return false;
	}
	AudioCodecContext = avcodec_alloc_context3(AudioCodec);
	if (!AudioCodecContext)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not allocate 'MP3' codec context!"));
		return false;
	}

	// setting video codec
	VideoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!VideoCodec)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find 'h264' codec!"));
		return false;
	}
	VideoCodecContext = avcodec_alloc_context3(VideoCodec);
	if (!VideoCodecContext)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not allocate 'h264' codec context!"));
		return false;
	}

	SetCodecParams();

	av_format_set_video_codec(OutputFormatContext, VideoCodec);
	av_format_set_audio_codec(OutputFormatContext, AudioCodec);

	return true;
}

bool FFMuxer::OpenCodecs()
{
	int ret;
	// Open Codec
	ret = avcodec_open2(AudioCodecContext, AudioCodec, nullptr);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not open 'MP3' codec!"));
		PrintError(ret);
		return false;
	}
	ret = avcodec_open2(VideoCodecContext, VideoCodec, &Dictionary);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not open 'h264' codec!"));
		PrintError(ret);
		return false;
	}

	return true;
}

void FFMuxer::SetCodecParams()
{
	// Video codec params
	const AVRational dst_fps = { FPS, 1 };

	VideoCodecContext->codec_tag = 0;
	VideoCodecContext->codec_id = AV_CODEC_ID_H264;
	VideoCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
	VideoCodecContext->width = width;
	VideoCodecContext->height = height;
	VideoCodecContext->gop_size = 12;
	VideoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
	VideoCodecContext->framerate = dst_fps;
	VideoCodecContext->time_base = av_inv_q(dst_fps);	

	// Audio codec params
	AudioCodecContext->codec_id = AV_CODEC_ID_MP3;
	AudioCodecContext->codec_type = AVMEDIA_TYPE_AUDIO;
	AudioCodecContext->sample_fmt = AV_SAMPLE_FMT_S16P;
	AudioCodecContext->sample_rate = 44100;
	AudioCodecContext->channel_layout = AV_CH_LAYOUT_STEREO;
	AudioCodecContext->channels = av_get_channel_layout_nb_channels(AudioCodecContext->channel_layout);
	AudioCodecContext->bit_rate = 64000;	

	// Set Dictionary	
	av_dict_set(&Dictionary, "profile", "high", 0);
	av_dict_set(&Dictionary, "preset", "superfast", 0);
	av_dict_set(&Dictionary, "tune", "zerolatency", 0);
}

bool FFMuxer::InitStreams()
{
	int ret;
	AVStream* AudioStream = avformat_new_stream(OutputFormatContext, AudioCodec);
	if (!AudioStream)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not allocate Audio stream!"));
		return false;
	}
	AVStream* VideoStream = avformat_new_stream(OutputFormatContext, VideoCodec);
	if (!VideoStream)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not allocate Video stream!"));
		return false;
	}

	// copy settings
	ret = avcodec_parameters_from_context(AudioStream->codecpar, AudioCodecContext);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not initialize stream codec parameters!"));
		PrintError(ret);
		return false;
	}

	ret = avcodec_parameters_from_context(VideoStream->codecpar, VideoCodecContext);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not initialize stream codec parameters!"));
		PrintError(ret);
		return false;
	}

	AudioStream->codec->codec_tag = 0;
	VideoStream->codec->codec_tag = 0;
	if (OutputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
	{
		AudioStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		VideoStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

	return true;
}

bool FFMuxer::WriteHeader()
{
	int ret = avformat_write_header(OutputFormatContext, &Dictionary);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Cant Write header!"));
		return false;
	}

	return true;
}

bool FFMuxer::WriteTrailer()
{
	int ret = av_write_trailer(OutputFormatContext);
	if (ret != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Cant Write Trailer"));
		return false;
	}
	
	return true;
}

AVRational FFMuxer::GetRational(int num, int den)
{
	AVRational rational = { num,den };
	return rational;
}

void FFMuxer::PrintError(int ErrorCode)
{
	char error_buf[256];
	av_make_error_string(error_buf, sizeof(error_buf), ErrorCode);
	FString ErrDescription{ error_buf };
	UE_LOG(LogTemp, Error, TEXT("Error code: %d"), ErrorCode);
	UE_LOG(LogTemp, Error, TEXT("Error desc: %s"), *ErrDescription);
}
