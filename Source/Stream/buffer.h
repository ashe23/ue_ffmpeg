#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>
#include <climits>

#include "CoreMinimal.h"

#include "BufferPolicies.h"

template<typename T, size_t buffsize = 1, typename Adder = BlockingAdder, typename Remover = BlockingRemover>
class Buffer;

using VideoBuffer = Buffer<TArray<FColor>, 30, RemoveOldElementsAdder, BlockingRemover>;
//using AudioBuffer = Buffer<int16, std::numeric_limits<uint64>::max()>;

/*
class Buffer represents a
thread safe buffer for producer 
consumer scenario usage.
*/
template<typename T, size_t buffsize, typename Adder, typename Remover>
class Buffer
{
	static_assert(std::is_copy_constructible<T>::value, "Buffer element must be copyable");
	static_assert(buffsize != 0, "Buffersize must be more than 0");
	static_assert(IsAdder<Adder, std::deque<T>>::Is == true, "not an Adder");
	static_assert(IsRemover<Remover, std::deque<T>>::Is == true, "not a Remover");

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

	void add(T& num) {
		UE_LOG(LogTemp, Log, TEXT("add, size = %d"), buffer_.size());
		Adder obj;
		obj(mu, cond, buffer_, buffsize, num);
		return;
	}

	T remove() {
		UE_LOG(LogTemp, Log, TEXT("remove, size = %d"), buffer_.size());
		Remover obj;
		T res = obj(mu, cond, buffer_);
		return res;
	}

private:
	Buffer() = default;
	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;

	std::mutex mu;
	std::condition_variable cond;

	std::deque<T> buffer_;
	const size_t size_ = buffsize;
};

