// Fill out your copyright notice in the Description page of Project Settings.

#include "FFMuxer.h"

void FFMuxer::Initialize()
{
	if (!initialized)
	{
		UE_LOG(LogTemp,Warning,TEXT("Initializing ffmpeg"));

		av_register_all();
		avformat_network_init();

		/* allocate the output media context */
		avformat_alloc_output_context2(&oc, nullptr, nullptr, OUTPUT_URL);
		if (!oc)
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not deduce output format from file extension: using MPEG."));
			avformat_alloc_output_context2(&oc, nullptr, "mpeg", OUTPUT_URL);
		}
		if (!oc)
		{
			return;
		}

		fmt = oc->oformat;

		/* Add the audio and video streams using the default format codecs
		* and initialize the codecs. */
		if (fmt->video_codec != AV_CODEC_ID_NONE) {
			if (!AddStream(&video_st, oc, &video_codec, fmt->video_codec))
			{
				UE_LOG(LogTemp, Error, TEXT("Initialized Failed"));
				return;
			}
			have_video = 1;
			encode_video = 1;
		}
		if (fmt->audio_codec != AV_CODEC_ID_NONE) {
			if (!AddStream(&audio_st, oc, &audio_codec, fmt->audio_codec)) 
			{
				UE_LOG(LogTemp, Error, TEXT("Initialized Failed"));
				return;
			}
			have_audio = 1;
			encode_audio = 1;
		}

		/* Now that all the parameters are set, we can open the audio and
		* video codecs and allocate the necessary encode buffers. */
		if (have_video) 
		{
			if (!OpenVideo(oc, video_codec, &video_st, opt))
			{
				return;
			}
		}

		if (have_audio)
		{
			if (!OpenAudio(oc, audio_codec, &audio_st, opt))
			{
				return;
			}

		}

		av_dump_format(oc, 0, OUTPUT_URL, 1);

		/* open the output file, if needed */
		if (!(fmt->flags & AVFMT_NOFILE))
		{
			ret = avio_open(&oc->pb, OUTPUT_URL, AVIO_FLAG_WRITE);
			if (ret < 0) 
			{
				UE_LOG(LogTemp, Error, TEXT("Could not open Output url"));
				return;
			}
		}

		/* Write the stream header, if any. */
		ret = avformat_write_header(oc, &opt);
		if (ret < 0)
		{
			UE_LOG(LogTemp, Error, TEXT("Error occurred when opening output file"));
			return;
		}	

		while (encode_video || encode_audio) {
			UE_LOG(LogTemp, Warning, TEXT("Muxing"));
			/* select the stream to encode */
			if (encode_video &&
				(!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base,
					audio_st.next_pts, audio_st.enc->time_base) <= 0)) {
				encode_video = !WriteVideoFrame(oc, &video_st);
			}
			else {
				encode_audio = !WriteAudioFrame(oc, &audio_st);
			}
		}

		/* Write the trailer, if any. The trailer must be written before you
		* close the CodecContexts open when you wrote the header; otherwise
		* av_write_trailer() may try to use memory that was freed on
		* av_codec_close(). */
		av_write_trailer(oc);

		/* Close each codec. */
		if (have_video)
			CloseStream(oc, &video_st);
		if (have_audio)
			CloseStream(oc, &audio_st);

		if (!(fmt->flags & AVFMT_NOFILE))
			/* Close the output file. */
			avio_closep(&oc->pb);

		/* free the stream */
		avformat_free_context(oc);

		// ========
		UE_LOG(LogTemp, Warning, TEXT("Initialized Successfully"));
		initialized = true;
	}
}

bool FFMuxer::IsInitialized() const
{
	return initialized;
}

void FFMuxer::Mux(FViewport* Viewport)
{
	if (Viewport)
	{
		MuxViewport = Viewport;
	}
	while (encode_video || encode_audio) {
		UE_LOG(LogTemp, Warning, TEXT("Muxing"));
		/* select the stream to encode */
		if (encode_video &&
			(!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base,
				audio_st.next_pts, audio_st.enc->time_base) <= 0)) {
			encode_video = !WriteVideoFrame(oc, &video_st);
		}
		else {
			encode_audio = !WriteAudioFrame(oc, &audio_st);
		}
	}
}

void FFMuxer::EndMux()
{
	/* Write the trailer, if any. The trailer must be written before you
	* close the CodecContexts open when you wrote the header; otherwise
	* av_write_trailer() may try to use memory that was freed on
	* av_codec_close(). */
	av_write_trailer(oc);

	/* Close each codec. */
	if (have_video)
		CloseStream(oc, &video_st);
	if (have_audio)
		CloseStream(oc, &audio_st);

	if (!(fmt->flags & AVFMT_NOFILE))
		/* Close the output file. */
		avio_closep(&oc->pb);

	/* free the stream */
	avformat_free_context(oc);
}

void FFMuxer::CloseStream(AVFormatContext *oc, OutputStream *ost)
{
	avcodec_free_context(&ost->enc);
	av_frame_free(&ost->frame);
	av_frame_free(&ost->tmp_frame);
	sws_freeContext(ost->sws_ctx);
	swr_free(&ost->swr_ctx);
}

bool FFMuxer::AddStream(OutputStream * ost, AVFormatContext * oc, AVCodec ** codec, AVCodecID codec_id)
{
	AVCodecContext *c;
	int i;

	/* find the encoder */
	*codec = avcodec_find_encoder(codec_id);
	if (!(*codec)) {
		UE_LOG(LogTemp, Error, TEXT("Could not find encoder"));
		return false;
	}

	ost->st = avformat_new_stream(oc, NULL);
	if (!ost->st) {
		UE_LOG(LogTemp, Error, TEXT("Could not allocate stream"));
		return false;
	}
	ost->st->id = oc->nb_streams - 1;
	c = avcodec_alloc_context3(*codec);
	if (!c) {
		UE_LOG(LogTemp, Error, TEXT("Could not alloc an encoding contex"));
		return false;		
	}
	ost->enc = c;

	switch ((*codec)->type) {
	case AVMEDIA_TYPE_AUDIO:
		c->sample_fmt = (*codec)->sample_fmts ?
			(*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
		c->bit_rate = 64000;
		c->sample_rate = 44100;
		if ((*codec)->supported_samplerates) {
			c->sample_rate = (*codec)->supported_samplerates[0];
			for (i = 0; (*codec)->supported_samplerates[i]; i++) {
				if ((*codec)->supported_samplerates[i] == 44100)
					c->sample_rate = 44100;
			}
		}
		c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
		c->channel_layout = AV_CH_LAYOUT_STEREO;
		if ((*codec)->channel_layouts) {
			c->channel_layout = (*codec)->channel_layouts[0];
			for (i = 0; (*codec)->channel_layouts[i]; i++) {
				if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
					c->channel_layout = AV_CH_LAYOUT_STEREO;
			}
		}
		c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
		ost->st->time_base = GetRational(1, c->sample_rate);
		break;

	case AVMEDIA_TYPE_VIDEO:
		c->codec_id = codec_id;

		c->bit_rate = 400000;
		/* Resolution must be a multiple of two. */
		c->width = 1280;
		c->height = 720; // todo: grab from viewport
		/* timebase: This is the fundamental unit of time (in seconds) in terms
		* of which frame timestamps are represented. For fixed-fps content,
		* timebase should be 1/framerate and timestamp increments should be
		* identical to 1. */
		ost->st->time_base = GetRational(1, STREAM_FRAME_RATE);
		c->time_base = ost->st->time_base;

		c->gop_size = 12; /* emit one intra frame every twelve frames at most */
		c->pix_fmt = STREAM_PIX_FMT;
		if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
			/* just for testing, we also add B-frames */
			c->max_b_frames = 2;
		}
		if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
			/* Needed to avoid using macroblocks in which some coeffs overflow.
			* This does not happen with normal video, it just happens here as
			* the motion of the chroma plane does not match the luma plane. */
			c->mb_decision = 2;
		}
		break;

	default:
		break;
	}

	/* Some formats want stream headers to be separate. */
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	return true;
}

bool FFMuxer::OpenVideo(AVFormatContext * oc, AVCodec * codec, OutputStream * ost, AVDictionary * opt_arg)
{
	int ret;
	AVCodecContext *c = ost->enc;
	AVDictionary *opt = NULL;

	av_dict_copy(&opt, opt_arg, 0);

	/* open the codec */
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not open video codec"));
		return false;
	}

	/* allocate and init a re-usable frame */
	ost->frame = AllocPicture(c->pix_fmt, c->width, c->height);
	if (!ost->frame)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not allocate video frame"));
		return false;		
	}

	/* If the output format is not YUV420P, then a temporary YUV420P
	* picture is needed too. It is then converted to the required
	* output format. */
	ost->tmp_frame = NULL;
	if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
		ost->tmp_frame = AllocPicture(AV_PIX_FMT_YUV420P, c->width, c->height);
		if (!ost->tmp_frame) {
			UE_LOG(LogTemp, Error, TEXT("Could not allocate temporary picture"));
			return false;			
		}
	}

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0) {
		UE_LOG(LogTemp, Error, TEXT("Could not copy the stream parameters"));
		return false;		
	}

	return true;
}

bool FFMuxer::OpenAudio(AVFormatContext * oc, AVCodec * codec, OutputStream * ost, AVDictionary * opt_arg)
{
	AVCodecContext *c;
	int nb_samples;
	int ret;
	AVDictionary *opt = NULL;

	c = ost->enc;

	/* open it */
	av_dict_copy(&opt, opt_arg, 0);
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		UE_LOG(LogTemp, Error, TEXT("Could not open audio codec"));
		return false;		
	}

	/* init signal generator */
	ost->t = 0;
	ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
	/* increment frequency by 110 Hz per second */
	ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

	if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
		nb_samples = 10000;
	else
		nb_samples = c->frame_size;

	ost->frame = AllocAudioFrame(c->sample_fmt, c->channel_layout,
		c->sample_rate, nb_samples);
	ost->tmp_frame = AllocAudioFrame(AV_SAMPLE_FMT_S16, c->channel_layout,
		c->sample_rate, nb_samples);

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0) 
	{
		UE_LOG(LogTemp, Error, TEXT("Could not copy the stream parameters"));
		return false;		
	}

	/* create resampler context */
	ost->swr_ctx = swr_alloc();
	if (!ost->swr_ctx) 
	{
		UE_LOG(LogTemp, Error, TEXT("Could not allocate resampler context"));
		return false;
	}

	/* set options */
	av_opt_set_int(ost->swr_ctx, "in_channel_count", c->channels, 0);
	av_opt_set_int(ost->swr_ctx, "in_sample_rate", c->sample_rate, 0);
	av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	av_opt_set_int(ost->swr_ctx, "out_channel_count", c->channels, 0);
	av_opt_set_int(ost->swr_ctx, "out_sample_rate", c->sample_rate, 0);
	av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

	/* initialize the resampling context */
	if ((ret = swr_init(ost->swr_ctx)) < 0) 
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to initialize the resampling context"));
		return false;		
	}

	return true;
}

AVFrame * FFMuxer::AllocPicture(AVPixelFormat pix_fmt, int width, int height)
{
	AVFrame *picture;
	int ret;

	picture = av_frame_alloc();
	if (!picture)
		return nullptr;

	picture->format = pix_fmt;
	picture->width = width;
	picture->height = height;

	/* allocate the buffers for the frame data */
	ret = av_frame_get_buffer(picture, 32);
	if (ret < 0) {
		UE_LOG(LogTemp, Error, TEXT("Could not allocate frame data"));
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
		UE_LOG(LogTemp, Error, TEXT("Error allocating an audio frame"));
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
			UE_LOG(LogTemp, Error, TEXT("Error allocating an audio buffer"));
			return nullptr;			
		}
	}

	return frame;
}

int FFMuxer::WriteVideoFrame(AVFormatContext * oc, OutputStream * ost)
{
	int ret;
	AVCodecContext *c;
	AVFrame *frame;
	int got_packet = 0;
	AVPacket pkt = { 0 };

	c = ost->enc;

	frame = GetVideoFrame(ost);

	av_init_packet(&pkt);

	/* encode the image */
	ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
	if (ret < 0) 
	{
		UE_LOG(LogTemp, Error, TEXT("Error encoding video frame"));
		return 1;
	}

	if (got_packet) 
	{
		ret = WriteFrame(oc, &c->time_base, ost->st, &pkt);
	}
	else
	{
		ret = 0;
	}

	if (ret < 0) 
	{
		UE_LOG(LogTemp, Error, TEXT("Error while writing video frame"));
	}

	return (frame || got_packet) ? 0 : 1;
}

int FFMuxer::WriteAudioFrame(AVFormatContext * oc, OutputStream * ost)
{
	AVCodecContext *c;
	AVPacket pkt = { 0 }; // data and size must be 0;
	AVFrame *frame;
	int ret;
	int got_packet;
	int dst_nb_samples;

	av_init_packet(&pkt);
	c = ost->enc;

	frame = GetAudioFrame(ost);

	if (frame) {
		/* convert samples from native format to destination codec format, using the resampler */
		/* compute destination number of samples */
		dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
			c->sample_rate, c->sample_rate, AV_ROUND_UP);
		av_assert0(dst_nb_samples == frame->nb_samples);

		/* when we pass a frame to the encoder, it may keep a reference to it
		* internally;
		* make sure we do not overwrite it here
		*/
		ret = av_frame_make_writable(ost->frame);
		if (ret < 0)
		{
			return 1;
		}

		/* convert to destination format */
		ret = swr_convert(ost->swr_ctx,
			ost->frame->data, dst_nb_samples,
			(const uint8_t **)frame->data, frame->nb_samples);
		if (ret < 0) 
		{
			UE_LOG(LogTemp, Error, TEXT("Error while converting"));
			return 1;
		}
		frame = ost->frame;

		frame->pts = av_rescale_q(ost->samples_count, GetRational(1, c->sample_rate), c->time_base);
		ost->samples_count += dst_nb_samples;
	}

	ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
	if (ret < 0) {
		UE_LOG(LogTemp, Error, TEXT("Error encoding audio frame"));
		return 1;
	}

	if (got_packet) {
		ret = WriteFrame(oc, &c->time_base, ost->st, &pkt);
		if (ret < 0) {
			UE_LOG(LogTemp, Error, TEXT("Error while writing audio frame"));
			return 1;
		}
	}

	return (frame || got_packet) ? 0 : 1;
}

int FFMuxer::WriteFrame(AVFormatContext * fmt_ctx, const AVRational * time_base, AVStream * st, AVPacket * pkt)
{
	/* rescale output packet timestamp values from codec to stream timebase */
	av_packet_rescale_ts(pkt, *time_base, st->time_base);
	pkt->stream_index = st->index;

	/* Write the compressed frame to the media file. */
	return av_interleaved_write_frame(fmt_ctx, pkt);
}

AVFrame * FFMuxer::GetVideoFrame(OutputStream * ost)
{
	AVCodecContext *c = ost->enc;

	/* check if we want to generate more frames */
	if (av_compare_ts(ost->next_pts, c->time_base,
		STREAM_DURATION, GetRational(1, 1)) >= 0)
		return nullptr;

	/* when we pass a frame to the encoder, it may keep a reference to it
	* internally; make sure we do not overwrite it here */
	if (av_frame_make_writable(ost->frame) < 0)
	{
		return nullptr;
	}

	if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
		/* as we only generate a YUV420P picture, we must convert it
		* to the codec pixel format if needed */
		if (!ost->sws_ctx) {
			ost->sws_ctx = sws_getContext(c->width, c->height,
				AV_PIX_FMT_YUV420P,
				c->width, c->height,
				c->pix_fmt,
				SWS_BICUBIC, NULL, NULL, NULL);
			if (!ost->sws_ctx) 
			{
				UE_LOG(LogTemp, Error, TEXT("Could not initialize the conversion context"));
				return nullptr;
			}
		}
		FillYUVImage(ost->tmp_frame, ost->next_pts, c->width, c->height);
		sws_scale(ost->sws_ctx,
			(const uint8_t * const *)ost->tmp_frame->data, ost->tmp_frame->linesize,
			0, c->height, ost->frame->data, ost->frame->linesize);
	}
	else {
		FillYUVImage(ost->frame, ost->next_pts, c->width, c->height);
	}

	ost->frame->pts = ost->next_pts++;

	return ost->frame;
}

AVFrame * FFMuxer::GetAudioFrame(OutputStream * ost)
{
	AVFrame *frame = ost->tmp_frame;
	int j, i, v;
	int16_t *q = (int16_t*)frame->data[0];

	/* check if we want to generate more frames */
	if (av_compare_ts(ost->next_pts, ost->enc->time_base,
		STREAM_DURATION, GetRational(1, 1)) >= 0)
		return NULL;

	for (j = 0; j <frame->nb_samples; j++) {
		v = (int)(sin(ost->t) * 10000);
		for (i = 0; i < ost->enc->channels; i++)
			*q++ = v;
		ost->t += ost->tincr;
		ost->tincr += ost->tincr2;
	}

	frame->pts = ost->next_pts;
	ost->next_pts += frame->nb_samples;

	return frame;
}

void FFMuxer::FillYUVImage(AVFrame * pict, int frame_index, int width, int height)
{
	//int x, y, i;

	//i = frame_index;

	/* Y */
	//for (y = 0; y < height; y++)
	//	for (x = 0; x < width; x++)
	//		pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

	///* Cb and Cr */
	//for (y = 0; y < height / 2; y++) {
	//	for (x = 0; x < width / 2; x++) {
	//		pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
	//		pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
	//	}
	//}

	// ===================
	if (MuxViewport) 
	{
		auto ViewportSize = MuxViewport->GetSizeXY();
		TArray<uint8> SingleFrameBuffer;
		TArray<FColor> ColorBuffer;
		if (!MuxViewport->ReadPixels(ColorBuffer, FReadSurfaceDataFlags(),
			FIntRect(0, 0, ViewportSize.X, ViewportSize.Y)))
		{
			UE_LOG(LogTemp, Error, TEXT("Cannot read from viewport.Aborting"));
			return;
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

		uint8* inputData = SingleFrameBuffer.GetData();

		// filling frame with actual data
		int InLineSize[1];
		InLineSize[0] = 4 * 1280;
		uint8* inData[1] = { SingleFrameBuffer.GetData() };
		pict->data[0] = inputData;

	}
	//sws_scale(swsctx, inData, InLineSize, 0, 720, frame->data, frame->linesize);
	//frame->pts += av_rescale_q(1, out_codec_ctx->time_base, out_stream->time_base);
}

AVRational FFMuxer::GetRational(int num, int den)
{
	AVRational rational = { num,den };
	return rational;
}
