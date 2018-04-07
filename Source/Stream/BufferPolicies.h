#pragma once

template<typename T, typename BufferT>
struct IsAdder
{
	typedef typename BufferT::value_type vType;
	template<typename U, typename BufferT, void(U::*)(std::mutex&, std::condition_variable&,
		BufferT&, size_t, vType) >
	struct SFINAE 
	{
	};

	template<typename U, typename BufferT> static char Test(SFINAE<U, BufferT, &U::operator()>*) {
		return char();
	}
	template<typename U, typename BufferT> static int Test(...) {
		return int();
	}
	static const bool Is = sizeof(Test<T, BufferT>(0)) == sizeof(char);
};

template<typename T, typename BufferT>
struct IsRemover
{
	typedef typename BufferT::value_type vType;
	template<typename U, typename BufferT, vType(U::*)(std::mutex&, std::condition_variable&, BufferT&) >
		struct SFINAE
	{
	};

	template<typename U, typename BufferT> static char Test(SFINAE<U, BufferT, &U::operator()>*) {
		return char();
	}
	template<typename U, typename BufferT> static int Test(...) {
		return int();
	}
	static const bool Is = sizeof(Test<T, BufferT>(0)) == sizeof(char);
};

/*
Blocking add Policy.
Thread waits if buffer
size reachs it limit.
*/
struct BlockingAdder
{
	template <typename BufferT>
	void operator() (std::mutex& lock, std::condition_variable& condVar, BufferT& buffer, size_t size, const typename BufferT::value_type e)
	{
		std::unique_lock<std::mutex> locker(lock);
		condVar.wait(locker, [&]() {return buffer.size() < size; });
		buffer.push_back(e);
		locker.unlock();
		condVar.notify_all();
	}
};

/*
Blocking remove Policy.
Thread waits if buffer
is empty.
*/
struct BlockingRemover
{
	template <typename BufferT>
	typename BufferT::value_type operator() (std::mutex& lock, std::condition_variable& condVar, BufferT& buffer)
	{
		std::unique_lock<std::mutex> locker(lock);
		condVar.wait(locker, [&]() {return buffer.size() > 0; });
		BufferT::value_type back = buffer.front();
		buffer.pop_front();
		locker.unlock();
		condVar.notify_all();
		return back;
	}
};

/*
Non blocking add Policy
which removes old elements
to make room for new ones.
*/
struct RemoveOldElementsAdder
{
	template <typename BufferT>
	void operator() (std::mutex& lock, std::condition_variable& condVar, BufferT& buffer, size_t size, const typename BufferT::value_type e)
	{
		std::unique_lock<std::mutex> locker(lock);
		if (buffer.size() == size) {
			buffer.erase(buffer.begin(), buffer.begin() + buffer.size() / 3);
		}
		buffer.push_back(e);
		locker.unlock();
		condVar.notify_all();
	}
};

/*
Non blocking add Policy
which removes every 3rd element
to make room for new ones.
*/
//struct RemoveOldElementsAdder
//{
//	template <typename BufferT>
//	void operator() (std::mutex& lock, std::condition_variable& condVar, BufferT& buffer, size_t size, const typename BufferT::value_type e)
//	{
//		std::unique_lock<std::mutex> locker(lock);
//		if (buffer.size() == size) {
//			// todo
//		}
//		buffer.push_back(e);
//		locker.unlock();
//		condVar.notify_all();
//	}
//};

/*
Non blocking remove Policy
which returns default constructed
element in case there are no
elements in buffer.
*/
struct NonBlockingRemover
{
	template <typename BufferT>
	typename BufferT::value_type operator() (std::mutex& lock, std::condition_variable& condVar, BufferT& buffer)
	{
		std::unique_lock<std::mutex> locker(lock);
		BufferT::value_type res;
		if (!buffer.empty())
		{
			res = buffer.front();
			buffer.pop_front();
		}
		locker.unlock();
		return res;
	}
};