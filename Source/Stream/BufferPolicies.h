#pragma once

/*
Non blocking add Policy
which removes old elements
to make room for new ones.
*/
struct RemoveOldElements
{
	template <typename Lock, typename CondVarT, typename BufferT, typename BufferSizeT, typename ElemT>
	void operator() (Lock& lock, CondVarT& condVar, BufferT& buffer, const BufferSizeT size, ElemT e)
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
//	template <typename Lock, typename CondVarT, typename BufferT, typename BufferSizeT, typename ElemT>
//	void operator() (Lock& lock, CondVarT& condVar, BufferT& buffer, const BufferSizeT size, ElemT e)
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
	template <typename Lock, typename CondVarT, typename BufferT>
	auto operator() (Lock& lock, CondVarT& condVar, BufferT& buffer)
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