#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>

#include "CoreMinimal.h"

template<typename T, int buffsize = 30>
class Buffer;

using VideoBuffer = Buffer<TArray<FColor>>;


/*
class Buffer represents a
thread safe buffer for producer 
consumer scenario usage.
*/
template<typename T, int buffsize>
class Buffer
{
public:
	/*
	Buffer<T>::GetInstance() creates
	buffer of T elements. T must be 
	TArray<FColor> in case of video buffer.
	*/
	static Buffer& GetInstance() {
		static Buffer instance;
		return instance;
	}

	/*
	Non blocking add funcion.
	Consider making policy of removing oldest
	elements as a template parameter
	*/
	void add(T& num) {
		UE_LOG(LogTemp, Log, TEXT("add, size = %d"), buffer_.size());
		while (true) {
			std::unique_lock<std::mutex> locker(mu);
			//cond.wait(locker, [this]() {return buffer_.size() < size_; });
			if (buffer_.size() == size_) {
				// remove oldest 10 elems
				buffer_.erase(buffer_.begin(), buffer_.begin() + 10);
			}

			buffer_.push_back(num);
			locker.unlock();
			cond.notify_all();
			return;
		}
	}
	/*
	Blocking remove function.
	Waits if buffer is empty.
	*/
	T remove() {
		UE_LOG(LogTemp, Log, TEXT("remove, size = %d"), buffer_.size());
		while (true) {
			std::unique_lock<std::mutex> locker(mu);
			cond.wait(locker, [this]() {return buffer_.size() > 0; });
			T back = buffer_.front();
			buffer_.pop_front();
			locker.unlock();
			cond.notify_all();
			return back;
		}
	}

private:
	Buffer() = default;
	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;

	std::mutex mu;
	std::condition_variable cond;

	std::deque<T> buffer_;
	const unsigned int size_ = buffsize;
};

