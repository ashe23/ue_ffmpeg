// Fill out your copyright notice in the Description page of Project Settings.

#include "FFMuxer.h"

void FFMuxer::Initialize(int32 Width, int32 Height)
{
	width = Width;
	height = Height;

	if (!initialized)
	{
		UE_LOG(LogTemp,Warning,TEXT("Initializing ffmpeg"));
		
		InitFFmpeg();

		if (InitOutputFormatContext() && InitIOContext())
		{
			if (InitCodecs())
			{				
				if (OpenCodecs())
				{
					/*AudioStream->codecpar->extradata = AudioCodecContext->extradata;
					AudioStream->codecpar->extradata_size = AudioCodecContext->extradata_size;*/
					VideoStream->codecpar->extradata = VideoCodecContext->extradata;
					VideoStream->codecpar->extradata_size = VideoCodecContext->extradata_size;

					av_dump_format(OutputFormatContext, 0, OUTPUT_URL, 1);

					if (InitSampleScaler() && AllocateFrames())
					{
						// Writing header
						if (WriteHeader())
						{

							tincr = 2 * M_PI * 110.0 / AudioCodecContext->sample_rate;
							tincr2 = 2 * M_PI * 110.0 / AudioCodecContext->sample_rate / AudioCodecContext->sample_rate;

							// Reading Audio data 
							// todo change later
							FString Path = FPaths::ProjectDir() + AudioFile;
							if (FFileHelper::LoadFileToArray(AudioBuffer, *Path))
							{
								UE_LOG(LogTemp, Warning, TEXT("Audio File Loaded, Size: %d"), AudioBuffer.Num());

								// Removing 44 bytes of headers info from audio file
								AudioBuffer.RemoveAt(0, 44);

								UE_LOG(LogTemp, Warning, TEXT("Size: %d"), AudioBuffer.Num());
							}
							else
							{
								UE_LOG(LogTemp, Error, TEXT("Cant load audio file"));
							}

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
		//UE_LOG(LogTemp, Warning, TEXT("Muxing"));

		int DecodingTime = av_compare_ts(CurrentVideoPTS, VideoCodecContext->time_base, CurrentAudioPTS, AudioCodecContext->time_base);
		//UE_LOG(LogTemp, Warning, TEXT("=========="));
		//UE_LOG(LogTemp, Warning, TEXT("Audio TimeBase: %d - %d"), AudioCodecContext->time_base.num, AudioCodecContext->time_base.den);
		//UE_LOG(LogTemp, Warning, TEXT("Video TimeBase: %d - %d"), VideoCodecContext->time_base.num, VideoCodecContext->time_base.den);
		//UE_LOG(LogTemp, Warning, TEXT("=========="));
		UE_LOG(LogTemp, Warning, TEXT("Frame Pushed: %d , AudioPTS: %d , VideoPTS: %d"), FramesPushed, CurrentAudioPTS, CurrentVideoPTS);
		if (DecodingTime <= 0) // video
		{
			// Writing video frame
			//UE_LOG(LogTemp, Warning, TEXT("Writing video frame"));
			CanStream = WriteVideoFrame(Viewport);
		}
		else // audio
		{
			// Writing audio frame
			UE_LOG(LogTemp, Warning, TEXT("Writing audio frame"));
			CanStream = WriteAudioFrame();
		}
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
	int ret = avformat_alloc_output_context2(&OutputFormatContext, nullptr, "flv", nullptr);
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
	if (!OutputFormatContext)
	{
		UE_LOG(LogTemp, Error, TEXT("Output context is NULL!"));
		CanStream = false;
		return false;
	}
	if (!(OutputFormatContext->oformat->flags & AVFMT_NOFILE))
	{
		int ret = avio_open2(&OutputFormatContext->pb, OUTPUT_URL, AVIO_FLAG_WRITE, nullptr, nullptr);
		if (ret < 0)
		{
			UE_LOG(LogTemp, Error, TEXT("Could not open output IO context!"));
			PrintError(ret);
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
	//AudioStream = avformat_new_stream(OutputFormatContext, nullptr);
	//if (!AudioStream)
	//{
	//	UE_LOG(LogTemp, Error, TEXT("Could not create new stream for audio!"));
	//	return false;
	//}
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

	VideoStream = avformat_new_stream(OutputFormatContext, VideoCodec);
	if (!VideoStream)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not create new stream for video!"));
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

bool FFMuxer::AllocateFrames()
{
	int ret;

	// audio
	AudioFrame = av_frame_alloc();
	if (!AudioFrame)
	{
		UE_LOG(LogTemp, Error, TEXT("Cant allocate frame for audio"));
		return false;
	}

	AudioFrame->format = AV_SAMPLE_FMT_S16P;
	AudioFrame->channel_layout = AudioCodecContext->channel_layout;
	AudioFrame->sample_rate = AudioCodecContext->sample_rate;
	AudioFrame->nb_samples = AudioCodecContext->frame_size;

	ret = av_frame_get_buffer(AudioFrame, 0);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Error allocate frame for audio"));
		return false;
	}

	// video
	VideoFrame = av_frame_alloc();
	if (!VideoFrame)
	{
		UE_LOG(LogTemp, Error, TEXT("Cant allocate frame for video"));
		return false;
	}

	VideoFrame->format = AV_PIX_FMT_YUV420P;
	VideoFrame->width = width;
	VideoFrame->height = height;

	ret = av_frame_get_buffer(VideoFrame, 32);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Error allocate frame for video"));
		PrintError(ret);
		return false;
	}

	return true;
}

bool FFMuxer::InitSampleScaler()
{
	SamplerContext = sws_getContext(width, height, AV_PIX_FMT_RGBA, width, height, VideoCodecContext->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
	if (!SamplerContext)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not initialize sample scaler!"));
		CanStream = false;
		return false;
	}

	return true;
}

bool FFMuxer::InitResampler()
{
	int ret;
	ResamplerContext = swr_alloc();
	if (!ResamplerContext) 
	{
		UE_LOG(LogTemp, Error, TEXT("Could not allocate resampler context"));
		return false;
	}

	/* set options */
	av_opt_set_int(ResamplerContext, "in_channel_count", AudioCodecContext->channels, 0);
	av_opt_set_int(ResamplerContext, "in_sample_rate", AudioCodecContext->sample_rate, 0);
	av_opt_set_sample_fmt(ResamplerContext, "in_sample_fmt", AV_SAMPLE_FMT_S16P, 0);
	av_opt_set_int(ResamplerContext, "out_channel_count", AudioCodecContext->channels, 0);
	av_opt_set_int(ResamplerContext, "out_sample_rate", AudioCodecContext->sample_rate, 0);
	av_opt_set_sample_fmt(ResamplerContext, "out_sample_fmt", AudioCodecContext->sample_fmt, 0);

	/* initialize the resampling context */
	if ((ret = swr_init(ResamplerContext)) < 0) 
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to initialize the resampling context"));
		return false;
	}

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

	if (OutputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
	{
		VideoCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	// Audio codec params
	AudioCodecContext->codec_id = AV_CODEC_ID_MP3;
	AudioCodecContext->codec_type = AVMEDIA_TYPE_AUDIO;
	AudioCodecContext->sample_fmt = AV_SAMPLE_FMT_S16P;
	AudioCodecContext->sample_rate = 44100;
	AudioCodecContext->channel_layout = AV_CH_LAYOUT_STEREO;
	AudioCodecContext->channels = av_get_channel_layout_nb_channels(AudioCodecContext->channel_layout);
	AudioCodecContext->bit_rate = 64000;	
	//const AVRational AudioTimeBase = { 1, AudioCodecContext->sample_rate };
	//AudioCodecContext->time_base = AudioTimeBase;

	/*if (OutputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
	{
		AudioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}*/

	// Set Dictionary	
	av_dict_set(&Dictionary, "profile", "high", 0);
	av_dict_set(&Dictionary, "preset", "superfast", 0);
	av_dict_set(&Dictionary, "tune", "zerolatency", 0);

	int ret = avcodec_parameters_from_context(VideoStream->codecpar, VideoCodecContext);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not initialize stream codec parameters!"));
		PrintError(ret);
		return;
	}
}

bool FFMuxer::InitStreams()
{
	int ret;
	AudioStream = avformat_new_stream(OutputFormatContext, AudioCodec);
	if (!AudioStream)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not allocate Audio stream!"));
		return false;
	}
	VideoStream = avformat_new_stream(OutputFormatContext, VideoCodec);
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

	/*AudioStream->codec->codec_tag = 0;
	VideoStream->codec->codec_tag = 0;
	if (OutputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
	{
		AudioStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		VideoStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}*/

	return true;
}

bool FFMuxer::WriteHeader()
{
	int ret = avformat_write_header(OutputFormatContext, nullptr);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Cant Write header!"));
		PrintError(ret);
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

bool FFMuxer::WriteVideoFrame(FViewport* Viewport)
{
	// Checking if viewport exists
	if (!Viewport)
	{
		UE_LOG(LogTemp, Error, TEXT("No viewport.Aborting"));
		return false;
	}

	// Determine viewport size
	FIntPoint ViewportSize = Viewport->GetSizeXY();
	if (ViewportSize.X == 0 || ViewportSize.Y == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No viewport size.Aborting"));
		return false;
	}

	if (ViewportSize.X % 2 != 0 || ViewportSize.Y % 2 != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid Screen size. Must divisible by 2!"));
		return false;
	}
	
	// Reading actual pixel data of single frame from viewport
	TArray<FColor> ColorBuffer;
	if (!Viewport->ReadPixels(ColorBuffer, FReadSurfaceDataFlags(),
		FIntRect(0, 0, ViewportSize.X, ViewportSize.Y)))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot read from viewport.Aborting"));
		return false;
	}	
		
	// converting from TArray to const uint8*
	SingleFrameBuffer.Empty();
	SingleFrameBuffer.SetNum(ColorBuffer.Num() * 4);
	uint8* DestPtr = nullptr;
	for (auto i = 0; i < ColorBuffer.Num(); i++)
	{
		DestPtr = &SingleFrameBuffer[i * 4];
		auto SrcPtr = ColorBuffer[i];
		*DestPtr++ = SrcPtr.R;
		*DestPtr++ = SrcPtr.G;
		*DestPtr++ = SrcPtr.B;
		*DestPtr++ = SrcPtr.A;
	}

	const uint8* inputData = SingleFrameBuffer.GetData();

	// filling frame with actual data
	int InLineSize[1];
	InLineSize[0] = 4 * VideoCodecContext->width;
	uint8* inData[1] = { SingleFrameBuffer.GetData() };
	sws_scale(SamplerContext, inData, InLineSize, 0, VideoCodecContext->height, VideoFrame->data, VideoFrame->linesize);
	
	auto n_res_pts = av_rescale_q(1, VideoCodecContext->time_base, VideoStream->time_base);
	VideoFrame->pts += n_res_pts;
	//CurrentVideoPTS = VideoFrame->pts;
	//UE_LOG(LogTemp, Warning, TEXT("Frame: %d , CurrentVideoPTS: %d"), FramesPushed, CurrentVideoPTS);
	FramesPushed++;
	return Encode(VideoFrame, EFrameType::Video);
}

bool FFMuxer::WriteAudioFrame()
{
	int j, i, v, ret;
	int dst_nb_samples;
	int16_t *q = (int16_t*)AudioFrame->data[0];

	for (j = 0; j < AudioFrame->nb_samples; j++) {
		v = (int)(sin(t) * 10000);
		for (i = 0; i < AudioFrame->channels; i++)
			*q++ = v;
		t += tincr;
		tincr += tincr2;
	}

	AudioFrame->pts = CurrentAudioPTS;
	CurrentAudioPTS += AudioFrame->nb_samples;

	if (AudioFrame)
	{
		dst_nb_samples = av_rescale_rnd(
			swr_get_delay(ResamplerContext, AudioCodecContext->sample_rate) + AudioFrame->nb_samples,
			AudioCodecContext->sample_rate,
			AudioCodecContext->sample_rate, 
			AV_ROUND_UP
		);
		av_assert0(dst_nb_samples == AudioFrame->nb_samples);
	}

	ret = av_frame_make_writable(AudioFrame);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Cant make frame writable"));
		return false;
	}
	ret = swr_convert(
		ResamplerContext,
		AudioFrame->data,
		dst_nb_samples,
		(const uint8_t **)AudioFrame->data,
		AudioFrame->nb_samples
		);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Error while resampling"));
		return false;
	}

	AVRational r = { 1, AudioCodecContext->sample_rate };		
	AudioFrame->pts += av_rescale_q(SamplesCount, r, AudioCodecContext->time_base);
	//CurrentAudioPTS = AudioFrame->pts;
	SamplesCount += dst_nb_samples;
	
	return Encode(AudioFrame , EFrameType::Audio);
}

bool FFMuxer::Encode(AVFrame * Frame, EFrameType Type)
{	
	AVPacket pkt = { 0 };
	av_init_packet(&pkt);
	int ret;

	if (Type == EFrameType::Video)
	{
		av_packet_rescale_ts(&pkt, VideoCodecContext->time_base, VideoStream->time_base);
		ret = avcodec_send_frame(VideoCodecContext, VideoFrame);
	}
	else if (Type == EFrameType::Audio)
	{
		av_packet_rescale_ts(&pkt, AudioCodecContext->time_base, AudioStream->time_base);
		ret = avcodec_send_frame(AudioCodecContext, AudioFrame);
	}

	if (ret < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Error sending frame to codec context!"));
		PrintError(ret);
		CanStream = false;
		return false;
	}


	if (Type == EFrameType::Video)
	{
		ret = avcodec_receive_packet(VideoCodecContext, &pkt);
	}
	else if (Type == EFrameType::Audio)
	{
		ret = avcodec_receive_packet(AudioCodecContext, &pkt);
	}

	if (ret < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Error receiving packet from codec context!!"));
		PrintError(ret);
		CanStream = false;
		return false;
	}

	//av_packet_rescale_ts(&pkt, VideoStream->time_base, VideoCodecContext->time_base);
	av_interleaved_write_frame(OutputFormatContext, &pkt);
	av_packet_unref(&pkt);

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
