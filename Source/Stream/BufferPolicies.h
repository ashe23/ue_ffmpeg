#pragma once


/*
Blocking add Policy.
Thread waits if buffer
size reachs it limit.
*/
struct AddBlocking
{
	template <typename BufferT, typename ElemT>
	void operator() (std::mutex& lock, std::condition_variable& condVar, BufferT& buffer, size_t size, ElemT e)
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
struct RemoveBlocking
{
	template <typename BufferT>
	auto operator() (std::mutex& lock, std::condition_variable& condVar, BufferT& buffer)
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
struct RemoveOldElements
{
	template <typename BufferT, typename ElemT>
	void operator() (std::mutex& lock, std::condition_variable& condVar, BufferT& buffer, size_t size, ElemT e)
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
//struct RemoveOldElements
//{
//	template <typename BufferT, typename ElemT>
//	void operator() (std::mutex& lock, std::condition_variable& condVar, BufferT& buffer, size_t size, ElemT e)
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
struct RemoveNonBlocking
{
	template <typename BufferT>
	auto operator() (std::mutex& lock, std::condition_variable& condVar, BufferT& buffer)
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