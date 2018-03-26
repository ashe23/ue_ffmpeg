// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


struct VideoFrame
{
	TArray<uint8> Data;
};

struct AudioFrame {
	TArray<int32> Data;
};