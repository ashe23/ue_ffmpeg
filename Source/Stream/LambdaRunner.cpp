// Fill out your copyright notice in the Description page of Project Settings.

#include "LambdaRunner.h"

uint64 LambdaRunner::ThreadNumber = 0;

LambdaRunner::LambdaRunner(TFunction<void()> InFunction)
{
	FunctionPointer = InFunction;
	Finished = false;
	Number = ThreadNumber;

	FString threadStatGroup = FString::Printf(TEXT("LambdaRunner%d"), ThreadNumber);
	Thread = FRunnableThread::Create(this, *threadStatGroup, 0, TPri_BelowNormal); //windows default = 8mb for thread, could specify more
	ThreadNumber++;
}

LambdaRunner::~LambdaRunner()
{
	delete Thread;
	Thread = NULL;
}

bool LambdaRunner::Init()
{
	return true;
}

uint32 LambdaRunner::Run()
{
	if (FunctionPointer)
	{
		FunctionPointer();
	}

	return 0;
}

void LambdaRunner::Stop()
{
	Finished = true;
}

void LambdaRunner::Exit()
{
	Finished = true;
	delete this;
}

void LambdaRunner::EnsureCompletion()
{
	Stop();
	Thread->WaitForCompletion();
}

LambdaRunner * LambdaRunner::RunLambdaOnBackGroundThread(TFunction<void()> InFunction)
{
	LambdaRunner* Runnable;
	
	if (FGenericPlatformProcess::SupportsMultithreading())
	{
		Runnable = new LambdaRunner(InFunction);
		return Runnable;
	}
	else
	{
		return nullptr;
	}
}

