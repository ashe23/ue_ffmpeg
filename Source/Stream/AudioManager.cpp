#include "AudioManager.h"

#include <iostream>

#include "Runtime/Core/Public/HAL/PlatformFilemanager.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Engine.h"
#include "buffer.h"

// wav offset
static const int32 OFFSET = 44;

bool operator==(const AudioPCM& l, const AudioPCM& r)
{
	return (l.getName() == r.getName());
}

uint32 GetTypeHash(const AudioPCM& obj)
{
	return GetTypeHash(obj.getName());
}

AudioPCM::AudioPCM(const FString& name) :
	mName(name)
{
	FString AudioFilePath = FPaths::ProjectDir() + "/ThirdParty/audio/" + name;
	if (GEngine)
	{
		if (FPaths::FileExists(AudioFilePath))
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Found '%s' File!"), *AudioFilePath));
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("File '%s' not found!"), *AudioFilePath));
		}
	}
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IFileHandle* FileHandle = PlatformFile.OpenRead(*AudioFilePath);

	if (FileHandle)
	{
		mSize = FileHandle->Size();
		mSize -= OFFSET;
		uint8* Buffer = new uint8[mSize];
		if (!FileHandle->Seek(OFFSET))
		{
			UE_LOG(LogTemp, Warning, TEXT("FileHandle->Seek failed"));
		}
		if (!FileHandle->Read(Buffer, mSize))
		{
			UE_LOG(LogTemp, Warning, TEXT("FileHandle->Read failed"));
		}

		mBuffer.Reserve(mSize);
		for (int i = 0 ; i < mSize; ++i)
		{
			mBuffer.Add(Buffer[i]);
		}

		// Close the file again
		delete FileHandle;
		delete[] Buffer;

		//this->readFilePackets(AudioFilePath);
	}
}

AudioPCM::AudioPCM(const AudioPCM& other) :
	mName(other.getName()),
	mSize(other.getSize()),
	mBuffer(other.getBuffer())
{
}

AudioPCM& AudioPCM::operator=(AudioPCM other)
{
	swap(*this, other);
	return *this;
}

AudioPCM::AudioPCM(AudioPCM&& other)
{
	swap(*this, other);
}

AudioPCM::~AudioPCM()
{
}

void AudioPCM::printAudioFrameInfo(const AVCodecContext * codecContext, const AVFrame * frame)
{
	// See the following to know what data type (unsigned char, short, float, etc) to use to access the audio data:
	// http://ffmpeg.org/doxygen/trunk/samplefmt_8h.html#af9a51ca15301871723577c730b5865c5
	UE_LOG(LogTemp, Warning, TEXT("Audio frame info:\nSample count: %d"), frame->nb_samples);
	UE_LOG(LogTemp, Warning, TEXT("Channel count: %d"), codecContext->channels);
	UE_LOG(LogTemp, Warning, TEXT("Format: %d"), av_get_sample_fmt_name(codecContext->sample_fmt));
	UE_LOG(LogTemp, Warning, TEXT("Bytes per sample: %d"), av_get_bytes_per_sample(codecContext->sample_fmt));
	UE_LOG(LogTemp, Warning, TEXT("Is planar ? %d"), av_sample_fmt_is_planar(codecContext->sample_fmt));


	if (codecContext->channels > AV_NUM_DATA_POINTERS && av_sample_fmt_is_planar(codecContext->sample_fmt))
	{
		UE_LOG(LogTemp, Warning, TEXT("The audio stream (and its frames) have too many channels to fit in\n"
			 "frame->data. Therefore, to access the audio data, you need to use\n"
			 "frame->extended_data to access the audio data. It's planar, so\n"
			 "each channel is in a different element. That is:\n"
			 "  frame->extended_data[0] has the data for channel 1\n"
			 "  frame->extended_data[1] has the data for channel 2\n"
			 "  etc.\n"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Either the audio data is not planar, or there is enough room in\n"
			"frame->data to store all the channels, so you can either use\n"
			"frame->data or frame->extended_data to access the audio data (they\n"
			"should just point to the same data).\n"));
	}

	UE_LOG(LogTemp, Warning, TEXT("If the frame is planar, each channel is in a different element.\n"
		"That is:\n"
	    "  frame->data[0]/frame->extended_data[0] has the data for channel 1\n"
		"  frame->data[1]/frame->extended_data[1] has the data for channel 2\n"
		"  etc.\n"));

	UE_LOG(LogTemp, Warning, TEXT("If the frame is packed (not planar), then all the data is in\n"
		"frame->data[0]/frame->extended_data[0] (kind of like how some\n"
		"image formats have RGB pixels packed together, rather than storing\n"
		" the red, green, and blue channels separately in different arrays.\n"));

}

int AudioPCM::readFilePackets(const FString& AudioFilePath)
{
	av_register_all();
	AVFrame* frame = av_frame_alloc();
	if (!frame)
	{
		std::cout << "Error allocating the frame" << std::endl;
		return 1;
	}

	// you can change the file name "01 Push Me to the Floor.wav" to whatever the file is you're reading, like "myFile.ogg" or
	// "someFile.webm" and this should still work
	AVFormatContext* formatContext = NULL;
	if (avformat_open_input(&formatContext, TCHAR_TO_UTF8(*AudioFilePath), NULL, NULL) != 0)
	{
		av_free(frame);
		std::cout << "Error opening the file" << std::endl;
		return 1;
	}

	if (avformat_find_stream_info(formatContext, NULL) < 0)
	{
		av_free(frame);
		avformat_close_input(&formatContext);
		std::cout << "Error finding the stream info" << std::endl;
		return 1;
	}

	// Find the audio stream
	AVCodec* cdc = nullptr;
	int streamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &cdc, 0);
	if (streamIndex < 0)
	{
		av_free(frame);
		avformat_close_input(&formatContext);
		std::cout << "Could not find any audio stream in the file" << std::endl;
		return 1;
	}

	AVStream* audioStream = formatContext->streams[streamIndex];
	AVCodecContext* codecContext = audioStream->codec;
	codecContext->codec = cdc;

	if (avcodec_open2(codecContext, codecContext->codec, NULL) != 0)
	{
		av_free(frame);
		avformat_close_input(&formatContext);
		std::cout << "Couldn't open the context with the decoder" << std::endl;
		return 1;
	}

	std::cout << "This stream has " << codecContext->channels << " channels and a sample rate of " << codecContext->sample_rate << "Hz" << std::endl;
	std::cout << "The data is in the format " << av_get_sample_fmt_name(codecContext->sample_fmt) << std::endl;

	AVPacket readingPacket;
	av_init_packet(&readingPacket);

	// Read the packets in a loop
	while (av_read_frame(formatContext, &readingPacket) == 0)
	{
		if (readingPacket.stream_index == audioStream->index)
		{
			AVPacket decodingPacket = readingPacket;

			// Audio packets can have multiple audio frames in a single packet
			while (decodingPacket.size > 0)
			{
				// Try to decode the packet into a frame
				// Some frames rely on multiple packets, so we have to make sure the frame is finished before
				// we can use it
				int gotFrame = 0;
				int result = avcodec_decode_audio4(codecContext, frame, &gotFrame, &decodingPacket);

				if (result >= 0 && gotFrame)
				{
					decodingPacket.size -= result;
					decodingPacket.data += result;

					// We now have a fully decoded audio frame
					printAudioFrameInfo(codecContext, frame);
				}
				else
				{
					decodingPacket.size = 0;
					decodingPacket.data = nullptr;
				}
			}
		}

		// You *must* call av_free_packet() after each call to av_read_frame() or else you'll leak memory
		av_free_packet(&readingPacket);
	}

	// Some codecs will cause frames to be buffered up in the decoding process. If the CODEC_CAP_DELAY flag
	// is set, there can be buffered up frames that need to be flushed, so we'll do that
	if (codecContext->codec->capabilities & CODEC_CAP_DELAY)
	{
		av_init_packet(&readingPacket);
		// Decode all the remaining frames in the buffer, until the end is reached
		int gotFrame = 0;
		while (avcodec_decode_audio4(codecContext, frame, &gotFrame, &readingPacket) >= 0 && gotFrame)
		{
			// We now have a fully decoded audio frame
			printAudioFrameInfo(codecContext, frame);
		}
	}

	// Clean up!
	av_free(frame);
	avcodec_close(codecContext);
	avformat_close_input(&formatContext);

	return 0;
}

void swap(AudioPCM& first, AudioPCM& second)
{
	using std::swap;
	swap(first.mName, second.mName);
	swap(first.mSize, second.mSize);
	swap(first.mBuffer, second.mBuffer);
}

void AudioManager::addAudioList(const TArray<FString>& filenames)
{
	for (auto v : filenames)
	{
		mAudioSet.Add(v, AudioPCM(v));
	}
}

AudioPCM AudioManager::getAudio(const FString & filename) const
{
	check(mAudioSet.Contains(filename));
	return mAudioSet[filename];
}

void AudioManager::Empty()
{
	mAudioSet.Empty();
}
