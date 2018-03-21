// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

extern "C"
{
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include "libavcodec/avcodec.h"

#include "libavformat/avformat.h"
#include "libavformat/avio.h"

#include "libavutil/avassert.h"
#include "libavutil/common.h"
#include "libavutil/frame.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/opt.h"
#include "libavutil/timestamp.h"
#include "libavutil/mathematics.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/channel_layout.h"
#include "libavutil/parseutils.h"
#include "libavutil/imgutils.h"
#include "libavutil/error.h"

#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}