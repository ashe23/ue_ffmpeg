// Fill out your copyright notice in the Description page of Project Settings.

#include "WaveFileManager.h"
#include "FileHelper.h"

static void WriteUInt32ToByteArrayLE(TArray<uint8>& InByteArray, int32& Index, const uint32 Value)
{
	InByteArray[Index++] = (uint8)(Value >> 0);
	InByteArray[Index++] = (uint8)(Value >> 8);
	InByteArray[Index++] = (uint8)(Value >> 16);
	InByteArray[Index++] = (uint8)(Value >> 24);
}

static void WriteUInt16ToByteArrayLE(TArray<uint8>& InByteArray, int32& Index, const uint16 Value)
{
	InByteArray[Index++] = (uint8)(Value >> 0);
	InByteArray[Index++] = (uint8)(Value >> 8);
}


void WaveFileManager::Serialize(TArray<uint8>& OutWaveFileData, const uint8 * InPCMData, const int32 NumBytes)
{
	// Reserve space for the raw wave data
	OutWaveFileData.Empty(NumBytes + 44);
	OutWaveFileData.AddZeroed(NumBytes + 44);

	int32 WaveDataByteIndex = 0;

	// Wave Format Serialization ----------

	// FieldName: ChunkID
	// FieldSize: 4 bytes
	// FieldValue: RIFF (FourCC value, big-endian)
	OutWaveFileData[WaveDataByteIndex++] = 'R';
	OutWaveFileData[WaveDataByteIndex++] = 'I';
	OutWaveFileData[WaveDataByteIndex++] = 'F';
	OutWaveFileData[WaveDataByteIndex++] = 'F';

	// ChunkName: ChunkSize: 4 bytes 
	// Value: NumBytes + 36. Size of the rest of the chunk following this number. Size of entire file minus 8 bytes.
	WriteUInt32ToByteArrayLE(OutWaveFileData, WaveDataByteIndex, NumBytes + 36);

	// FieldName: Format 
	// FieldSize: 4 bytes
	// FieldValue: "WAVE"  (big-endian)
	OutWaveFileData[WaveDataByteIndex++] = 'W';
	OutWaveFileData[WaveDataByteIndex++] = 'A';
	OutWaveFileData[WaveDataByteIndex++] = 'V';
	OutWaveFileData[WaveDataByteIndex++] = 'E';

	// FieldName: Subchunk1ID
	// FieldSize: 4 bytes
	// FieldValue: "fmt "
	OutWaveFileData[WaveDataByteIndex++] = 'f';
	OutWaveFileData[WaveDataByteIndex++] = 'm';
	OutWaveFileData[WaveDataByteIndex++] = 't';
	OutWaveFileData[WaveDataByteIndex++] = ' ';

	// FieldName: Subchunk1Size
	// FieldSize: 4 bytes
	// FieldValue: 16 for PCM
	WriteUInt32ToByteArrayLE(OutWaveFileData, WaveDataByteIndex, 16);

	// FieldName: AudioFormat
	// FieldSize: 2 bytes
	// FieldValue: 1 for PCM
	WriteUInt16ToByteArrayLE(OutWaveFileData, WaveDataByteIndex, 1);

	// FieldName: NumChannels
	// FieldSize: 2 bytes
	// FieldValue: 1 for for mono
	WriteUInt16ToByteArrayLE(OutWaveFileData, WaveDataByteIndex, NumChannels);

	// FieldName: SampleRate
	// FieldSize: 4 bytes
	// FieldValue: MIC_SAMPLE_RATE
	WriteUInt32ToByteArrayLE(OutWaveFileData, WaveDataByteIndex, SampleRate);

	// FieldName: ByteRate
	// FieldSize: 4 bytes
	// FieldValue: SampleRate * NumChannels * BitsPerSample/8
	int32 ByteRate = SampleRate * NumChannels * 2;
	WriteUInt32ToByteArrayLE(OutWaveFileData, WaveDataByteIndex, ByteRate);

	// FieldName: BlockAlign
	// FieldSize: 2 bytes
	// FieldValue: NumChannels * BitsPerSample/8
	int32 BlockAlign = 2;
	WriteUInt16ToByteArrayLE(OutWaveFileData, WaveDataByteIndex, BlockAlign);

	// FieldName: BitsPerSample
	// FieldSize: 2 bytes
	// FieldValue: 16 (16 bits per sample)
	WriteUInt16ToByteArrayLE(OutWaveFileData, WaveDataByteIndex, 16);

	// FieldName: Subchunk2ID
	// FieldSize: 4 bytes
	// FieldValue: "data" (big endian)

	OutWaveFileData[WaveDataByteIndex++] = 'd';
	OutWaveFileData[WaveDataByteIndex++] = 'a';
	OutWaveFileData[WaveDataByteIndex++] = 't';
	OutWaveFileData[WaveDataByteIndex++] = 'a';

	// FieldName: Subchunk2Size
	// FieldSize: 4 bytes
	// FieldValue: number of bytes of the data
	WriteUInt32ToByteArrayLE(OutWaveFileData, WaveDataByteIndex, NumBytes);

	// Copy the raw PCM data to the audio file
	FMemory::Memcpy(&OutWaveFileData[WaveDataByteIndex], InPCMData, NumBytes);

	// Saving to .wav file
	if (FFileHelper::SaveArrayToFile(OutWaveFileData, *SavePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Saved Successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not save file"));
	}
}


void WaveFileManager::SetSampleRate(int32 SR)
{
	SampleRate = SR;
}

void WaveFileManager::SetNumChannels(int32 CH)
{
	NumChannels = CH;
}

