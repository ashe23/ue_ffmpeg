// Fill out your copyright notice in the Description page of Project Settings.

#include "FFMuxer.h"

#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 30 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

#define SCALE_FLAGS SWS_BICUBIC

 AVRational GetRational(int num, int den)
{
	AVRational rational = { num, den };
	return rational;
}
 

FFMuxer::~FFMuxer()
{
	Release();
}

void FFMuxer::Initialize(int32 Width, int32 Height)
{
	if (!initialized)
	{
		PrintEngineWarning("Initializing ffmpeg");
		initialized = true;
		width = Width;
		height = Height;

		av_register_all();

		avformat_alloc_output_context2(&FormatContext, nullptr, nullptr, filename);
		if (!FormatContext)
		{
			UE_LOG(LogTemp, Error, TEXT("Cant allocate output context"));
			return;
		}
		OutputFormat = FormatContext->oformat;

		// add stream for video
		if (OutputFormat->video_codec != AV_CODEC_ID_NONE)
		{
			AddVideoStream();
			have_video = 1;
			encode_video = 1;
		}

		if (OutputFormat->audio_codec != AV_CODEC_ID_NONE)
		{
			AddAudioStream();
			have_audio = 1;
			encode_audio = 1;
		}

		/* Now that all the parameters are set, we can open the audio and
		* video codecs and allocate the necessary encode buffers. */
		if (have_video)
		{
			OpenVideo();
		}

		if (have_audio) 
		{
			OpenAudio();
		}

		/* open the output file, if needed */
		if (!(OutputFormat->flags & AVFMT_NOFILE))
		{
			ret = avio_open(&FormatContext->pb, filename, AVIO_FLAG_WRITE);
			if (ret < 0)
			{
				PrintEngineError("Could not open filename");
				return;
			}
		}

		/* Write the stream header, if any. */
		ret = avformat_write_header(FormatContext, nullptr);
		if (ret < 0) 
		{
			PrintEngineError("Error occurred when opening output file");
			return;
		}		

		while (encode_video || encode_audio)
		{
			/* select the stream to encode */

			UE_LOG(LogTemp, Warning, TEXT("AudioPTS: %d , VideoPTS: %d"), audio_st.next_pts, video_st.next_pts);

			int DecodeTime = av_compare_ts(video_st.next_pts, video_st.enc->time_base, audio_st.next_pts, audio_st.enc->time_base);
			if (encode_video && (!encode_audio || DecodeTime <= 0))
			{
				encode_video = !WriteVideoFrame();
			}
			else
			{
				encode_audio = !WriteAudioFrame();
			}
		}

		/* Write the trailer, if any. The trailer must be written before you
		* close the CodecContexts open when you wrote the header; otherwise
		* av_write_trailer() may try to use memory that was freed on
		* av_codec_close(). */
		av_write_trailer(FormatContext);
		
		CanStream = true;		
		PrintEngineWarning("Initializing success");
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

void FFMuxer::Mux(FViewport * Viewport)
{
	while (encode_video || encode_audio)
	{
		/* select the stream to encode */

		UE_LOG(LogTemp,Warning,TEXT("AudioPTS: %d , VideoPTS: %d"), audio_st.next_pts, video_st.next_pts);

		int DecodeTime = av_compare_ts(video_st.next_pts, video_st.enc->time_base, audio_st.next_pts, audio_st.enc->time_base);
		if (encode_video && (!encode_audio || DecodeTime <= 0)) 
		{
			encode_video = !WriteVideoFrame();
		}
		else
		{
			encode_audio = !WriteAudioFrame();
		}
	}

	/* Write the trailer, if any. The trailer must be written before you
	* close the CodecContexts open when you wrote the header; otherwise
	* av_write_trailer() may try to use memory that was freed on
	* av_codec_close(). */
	av_write_trailer(FormatContext);	
}

void FFMuxer::Release()
{
	PrintEngineWarning("Releasing ffmpeg resources!");

	/* Close each codec. */
	if (have_video)
	{
		CloseVideoStream();
	}
	if (have_audio)
	{
		CloseAudioStream();
	}

	if (!(OutputFormat->flags & AVFMT_NOFILE))
	{
		/* Close the output file. */
		avio_closep(&FormatContext->pb);
	}

	/* free the stream */
	avformat_free_context(FormatContext);
}

void FFMuxer::PrintEngineError(FString ErrorString)
{
	UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorString);
}

void FFMuxer::PrintEngineWarning(FString Text)
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Text);
}

void FFMuxer::AddVideoStream()
{
	// allocation
	VideoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!VideoCodec)
	{
		PrintEngineError("Cant find encoder");
		return;
	}

	video_st.st = avformat_new_stream(FormatContext, VideoCodec);
	if (!video_st.st)
	{
		PrintEngineError("Cant create video stream");
		return;
	}

	video_st.st->id = FormatContext->nb_streams - 1;
	video_st.enc = avcodec_alloc_context3(VideoCodec);
	if (!video_st.enc)
	{
		PrintEngineError("Could not allocate encoding context");
		return;
	}

	// setting params
	video_st.enc->codec_id = VideoCodec->id;
	video_st.enc->bit_rate = 400000;
	/* Resolution must be a multiple of two. */
	video_st.enc->width = width;
	video_st.enc->height = height;
	/* timebase: This is the fundamental unit of time (in seconds) in terms
	* of which frame timestamps are represented. For fixed-fps content,
	* timebase should be 1/framerate and timestamp increments should be
	* identical to 1. */
	video_st.st->time_base = GetRational(1, STREAM_FRAME_RATE);
	video_st.enc->time_base = video_st.st->time_base;

	video_st.enc->gop_size = 12; /* emit one intra frame every twelve frames at most */
	video_st.enc->pix_fmt = STREAM_PIX_FMT;
	if (video_st.enc->codec_id == AV_CODEC_ID_MPEG2VIDEO)
	{
		/* just for testing, we also add B-frames */
		video_st.enc->max_b_frames = 2;
	}
	if (video_st.enc->codec_id == AV_CODEC_ID_MPEG1VIDEO) 
	{
		/* Needed to avoid using macroblocks in which some coeffs overflow.
		* This does not happen with normal video, it just happens here as
		* the motion of the chroma plane does not match the luma plane. */
		video_st.enc->mb_decision = 2;
	}

	/* Some formats want stream headers to be separate. */
	if (FormatContext->oformat->flags & AVFMT_GLOBALHEADER)
	{
		video_st.enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}
}

void FFMuxer::AddAudioStream()
{
	int i;
	// allocation
	AudioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!AudioCodec)
	{
		PrintEngineError("Cant find encoder");
		return;
	}

	audio_st.st = avformat_new_stream(FormatContext, AudioCodec);
	if (!video_st.st)
	{
		PrintEngineError("Cant create audio stream");
		return;
	}

	audio_st.st->id = FormatContext->nb_streams - 1;
	audio_st.enc = avcodec_alloc_context3(AudioCodec);
	if (!audio_st.enc)
	{
		PrintEngineError("Could not allocate encoding context");
		return;
	}

	// set params
	audio_st.enc->sample_fmt = AudioCodec->sample_fmts ? AudioCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
	audio_st.enc->bit_rate = 64000;
	audio_st.enc->sample_rate = 44100;
	if (AudioCodec->supported_samplerates) 
	{
		audio_st.enc->sample_rate = AudioCodec->supported_samplerates[0];
		for (i = 0; AudioCodec->supported_samplerates[i]; i++) 
		{
			if (AudioCodec->supported_samplerates[i] == 44100)
			{
				audio_st.enc->sample_rate = 44100;
			}
		}
	}
	audio_st.enc->channels = av_get_channel_layout_nb_channels(audio_st.enc->channel_layout);
	audio_st.enc->channel_layout = AV_CH_LAYOUT_STEREO;
	if (AudioCodec->channel_layouts) 
	{
		audio_st.enc->channel_layout = AudioCodec->channel_layouts[0];
		for (i = 0; AudioCodec->channel_layouts[i]; i++)
		{
			if (AudioCodec->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
			{
				audio_st.enc->channel_layout = AV_CH_LAYOUT_STEREO;
			}
		}
	}
	audio_st.enc->channels = av_get_channel_layout_nb_channels(audio_st.enc->channel_layout);
	audio_st.st->time_base = GetRational(1, audio_st.enc->sample_rate);
}

void FFMuxer::OpenVideo()
{
	int ret;

	av_dict_set(&Dictionary, "profile", "high", 0);
	av_dict_set(&Dictionary, "preset", "superfast", 0);
	av_dict_set(&Dictionary, "tune", "zerolatency", 0);

	//video_st.next_pts = 0;

	/* open the codec */
	ret = avcodec_open2(video_st.enc, VideoCodec, &Dictionary);
	av_dict_free(&Dictionary);

	if (ret < 0)
	{
		PrintEngineError("Could not open video codec");
		return;
	}

	/* allocate and init a re-usable frame */
	video_st.frame = AllocPicture(video_st.enc->pix_fmt, video_st.enc->width, video_st.enc->height);
	if (!video_st.frame) 
	{
		PrintEngineError("Could not allocate video frame");
		return;
	}

	/* If the output format is not YUV420P, then a temporary YUV420P
	* picture is needed too. It is then converted to the required
	* output format. */
	video_st.tmp_frame = nullptr;
	if (video_st.enc->pix_fmt != AV_PIX_FMT_YUV420P) 
	{
		video_st.tmp_frame = AllocPicture(AV_PIX_FMT_YUV420P, video_st.enc->width, video_st.enc->height);
		if (!video_st.tmp_frame)
		{
			PrintEngineError("Could not allocate temporary picture");
			return;
		}
	}

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(video_st.st->codecpar, video_st.enc);
	if (ret < 0)
	{
		PrintEngineError("Could not copy the stream parameters");
		return;
	}
}

void FFMuxer::OpenAudio()
{
	int nb_samples;
	int ret;

	ret = avcodec_open2(audio_st.enc, AudioCodec, nullptr);
	if (ret < 0) 
	{
		PrintEngineError("Could not open audio codec");
		return;
	}

	/* init signal generator */
	audio_st.t = 0;
	audio_st.tincr = 2 * M_PI * 110.0 / audio_st.enc->sample_rate;
	/* increment frequency by 110 Hz per second */
	audio_st.tincr2 = 2 * M_PI * 110.0 / audio_st.enc->sample_rate / audio_st.enc->sample_rate;
	//audio_st.next_pts = 0;

	if (audio_st.enc->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
	{
		nb_samples = 10000;
	}
	else
	{
		nb_samples = audio_st.enc->frame_size;
	}

	audio_st.frame = AllocAudioFrame(audio_st.enc->sample_fmt, audio_st.enc->channel_layout, audio_st.enc->sample_rate, nb_samples);
	audio_st.tmp_frame = AllocAudioFrame(AV_SAMPLE_FMT_S16, audio_st.enc->channel_layout, audio_st.enc->sample_rate, nb_samples);

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(audio_st.st->codecpar, audio_st.enc);
	if (ret < 0) 
	{
		PrintEngineError("Could not copy the stream parameters");
		return;
	}

	/* create resampler context */
	audio_st.swr_ctx = swr_alloc();
	if (!audio_st.swr_ctx) 
	{
		PrintEngineError("Could not allocate resampler context");
		return;
	}

	/* set options */
	av_opt_set_int(audio_st.swr_ctx, "in_channel_count", audio_st.enc->channels, 0);
	av_opt_set_int(audio_st.swr_ctx, "in_sample_rate", audio_st.enc->sample_rate, 0);
	av_opt_set_sample_fmt(audio_st.swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	av_opt_set_int(audio_st.swr_ctx, "out_channel_count", audio_st.enc->channels, 0);
	av_opt_set_int(audio_st.swr_ctx, "out_sample_rate", audio_st.enc->sample_rate, 0);
	av_opt_set_sample_fmt(audio_st.swr_ctx, "out_sample_fmt", audio_st.enc->sample_fmt, 0);

	/* initialize the resampling context */
	if ((ret = swr_init(audio_st.swr_ctx)) < 0) 
	{
		PrintEngineError("Failed to initialize the resampling context");
		return;
	}
}

AVFrame * FFMuxer::AllocPicture(AVPixelFormat pix_fmt, int w, int h)
{
	AVFrame *picture;
	int ret;

	picture = av_frame_alloc();
	if (!picture)
	{
		return nullptr;
	}

	picture->format = pix_fmt;
	picture->width = w;
	picture->height = h;

	/* allocate the buffers for the frame data */
	ret = av_frame_get_buffer(picture, 32);
	if (ret < 0) 
	{
		PrintEngineError("Could not allocate frame data");
		return nullptr;
	}

	return picture;
}

AVFrame * FFMuxer::AllocAudioFrame(AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples)
{
	AVFrame *frame = av_frame_alloc();
	int ret;

	if (!frame) 
	{
		PrintEngineError("Error allocating an audio frame");
		return nullptr;
	}

	frame->format = sample_fmt;
	frame->channel_layout = channel_layout;
	frame->sample_rate = sample_rate;
	frame->nb_samples = nb_samples;

	if (nb_samples) 
	{
		ret = av_frame_get_buffer(frame, 0);
		if (ret < 0)
		{
			PrintEngineError("Error allocating an audio buffer");
			return nullptr;
		}
	}

	return frame;
}

int FFMuxer::WriteVideoFrame()
{
	int ret;
	AVFrame *frame;
	int got_packet = 0;
	AVPacket pkt = { 0 };

	frame = GetVideoFrame();

	av_init_packet(&pkt);

	/* encode the image */
	ret = avcodec_encode_video2(video_st.enc, &pkt, frame, &got_packet);
	if (ret < 0)
	{
		PrintEngineError("Error encoding video frame");
		return 0;
	}

	if (got_packet)
	{
		ret = WriteFrame(&video_st.enc->time_base, video_st.st, &pkt);
	}
	else
	{
		ret = 0;
	}

	if (ret < 0) 
	{
		PrintEngineError("Error while writing video frame");
	}

	return (frame || got_packet) ? 0 : 1;
}

int FFMuxer::WriteAudioFrame()
{
	AVPacket pkt = { 0 }; // data and size must be 0;
	AVFrame *frame;
	int ret;
	int got_packet;
	int dst_nb_samples;

	av_init_packet(&pkt);

	frame = GetAudioFrame();

	if (frame) 
	{
		/* convert samples from native format to destination codec format, using the resampler */
		/* compute destination number of samples */
		dst_nb_samples = av_rescale_rnd(swr_get_delay(audio_st.swr_ctx, audio_st.enc->sample_rate) + frame->nb_samples, audio_st.enc->sample_rate, audio_st.enc->sample_rate, AV_ROUND_UP);
		av_assert0(dst_nb_samples == frame->nb_samples);

		/* when we pass a frame to the encoder, it may keep a reference to it
		* internally;
		* make sure we do not overwrite it here
		*/
		ret = av_frame_make_writable(audio_st.frame);
		if (ret < 0)
		{
			PrintEngineError("Cant make audio frame writable");
			return 0;
		}

		/* convert to destination format */
		ret = swr_convert(audio_st.swr_ctx,audio_st.frame->data, dst_nb_samples,(const uint8_t **)frame->data, frame->nb_samples);
		if (ret < 0) 
		{
			PrintEngineError("Error while converting");
			return 0;
		}
		frame = audio_st.frame;

		frame->pts = av_rescale_q(audio_st.samples_count, GetRational(1, audio_st.enc->sample_rate), audio_st.enc->time_base);
		audio_st.samples_count += dst_nb_samples;
	}

	ret = avcodec_encode_audio2(audio_st.enc, &pkt, frame, &got_packet);
	if (ret < 0) 
	{
		PrintEngineError("Error encoding audio frame");
		return 0;
	}

	if (got_packet) 
	{
		ret = WriteFrame(&audio_st.enc->time_base, audio_st.st, &pkt);
		if (ret < 0) 
		{
			PrintEngineError("Error while writing audio frame");
			return 0;
		}
	}

	return (frame || got_packet) ? 0 : 1;
}

int FFMuxer::WriteFrame(const AVRational * time_base, AVStream * st, AVPacket * pkt)
{
	/* rescale output packet timestamp values from codec to stream timebase */
	av_packet_rescale_ts(pkt, *time_base, st->time_base);
	pkt->stream_index = st->index;

	/* Write the compressed frame to the media file. */
	return av_interleaved_write_frame(FormatContext, pkt);
}

AVFrame * FFMuxer::GetVideoFrame()
{
	/* check if we want to generate more frames */
	if (av_compare_ts(video_st.next_pts, video_st.enc->time_base, STREAM_DURATION, GetRational(1, 1)) >= 0)
	{
		return nullptr;
	}

	/* when we pass a frame to the encoder, it may keep a reference to it
	* internally; make sure we do not overwrite it here */
	if (av_frame_make_writable(video_st.frame) < 0)
	{
		PrintEngineError("Cant Make Frame Writable");
		return nullptr;
	}

	if (video_st.enc->pix_fmt != AV_PIX_FMT_YUV420P)
	{
		/* as we only generate a YUV420P picture, we must convert it
		* to the codec pixel format if needed */
		if (!video_st.sws_ctx)
		{
			video_st.sws_ctx = sws_getContext(video_st.enc->width, video_st.enc->height,
				AV_PIX_FMT_YUV420P,
				video_st.enc->width, video_st.enc->height,
				video_st.enc->pix_fmt,
				SCALE_FLAGS, NULL, NULL, NULL);
			if (!video_st.sws_ctx)
			{
				PrintEngineError("Could not initialize the conversion context");
				return nullptr;
			}
		}
		FillYUVImage(video_st.tmp_frame, video_st.next_pts, video_st.enc->width, video_st.enc->height);
		sws_scale(video_st.sws_ctx, (const uint8_t * const *)video_st.tmp_frame->data, video_st.tmp_frame->linesize,0, video_st.enc->height, video_st.frame->data, video_st.frame->linesize);
	}
	else {
		FillYUVImage(video_st.frame, video_st.next_pts, video_st.enc->width, video_st.enc->height);
	}

	video_st.frame->pts = video_st.next_pts++;

	return video_st.frame;
}

AVFrame * FFMuxer::GetAudioFrame()
{
	int j, i, v;
	int16_t *q = (int16_t*)audio_st.tmp_frame->data[0];

	/* check if we want to generate more frames */
	if (av_compare_ts(audio_st.next_pts, audio_st.enc->time_base, STREAM_DURATION, GetRational(1, 1)) >= 0)
	{
		return nullptr;
	}

	for (j = 0; j <audio_st.tmp_frame->nb_samples; j++)
	{
		v = (int)(sin(audio_st.t) * 10000);
		for (i = 0; i < audio_st.enc->channels; i++)
		{
			*q++ = v;
		}
		audio_st.t += audio_st.tincr;
		audio_st.tincr += audio_st.tincr2;
	}

	audio_st.tmp_frame->pts = audio_st.next_pts;
	audio_st.next_pts += audio_st.tmp_frame->nb_samples;

	return audio_st.tmp_frame;
}

void FFMuxer::CloseVideoStream()
{
	avcodec_free_context(&video_st.enc);
	av_frame_free(&video_st.frame);
	av_frame_free(&video_st.tmp_frame);
	sws_freeContext(video_st.sws_ctx);
	swr_free(&video_st.swr_ctx);
}

void FFMuxer::CloseAudioStream()
{
	avcodec_free_context(&audio_st.enc);
	av_frame_free(&audio_st.frame);
	av_frame_free(&audio_st.tmp_frame);
	sws_freeContext(audio_st.sws_ctx);
	swr_free(&audio_st.swr_ctx);
}

void FFMuxer::FillYUVImage(AVFrame * pict, int frame_index, int width, int height)
{
	int x, y, i;

	i = frame_index;

	/* Y */
	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++)
			pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

	/* Cb and Cr */
	for (y = 0; y < height / 2; y++) {
		for (x = 0; x < width / 2; x++) {
			pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
			pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
		}
	}
}
