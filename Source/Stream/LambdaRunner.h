// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Core/Public/HAL/ThreadSafeBool.h"
#include "Runtime/Core/Public/HAL/RunnableThread.h"
#include "Runtime/Core/Public/HAL/Runnable.h"
#include "Runtime/Core/Public/GenericPlatform/GenericPlatformProcess.h"

/**
 * 
 */
class STREAM_API LambdaRunner : public FRunnable
{
private:
	/** Thread to run the worker FRunnable on */
	FRunnableThread * Thread;
	uint64 Number;

	//Lambda function pointer
	TFunction< void()> FunctionPointer;

	/** Use this thread-safe boolean to allow early exits for your threads */
	FThreadSafeBool Finished;

	//static TArray<FLambdaRunnable*> Runnables;
	static uint64 ThreadNumber;
public:
	//~~~ Thread Core Functions ~~~

	//Constructor / Destructor
	LambdaRunner(TFunction< void()> InFunction);
	virtual ~LambdaRunner();

	// Begin FRunnable interface.
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	virtual void Exit() override;
	// End FRunnable interface

	/** Makes sure this thread has stopped properly */
	void EnsureCompletion();

	/*
	Runs the passed lambda on the background thread, new thread per call
	*/
	static LambdaRunner* RunLambdaOnBackGroundThread(TFunction< void()> InFunction);
};
