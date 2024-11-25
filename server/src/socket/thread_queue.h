#ifndef THREAD_QUEUE_H
#define THREAD_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

template<class T>
class thread_queue
{
public:
	void push(T val)
	{
		std::lock_guard<std::mutex> lock(mut);
		que.push(val);
		cond.notify_one();
	}
	T pop()
	{
		std::unique_lock<std::mutex> lock(mut);
		while(que.empty())
			cond.wait(lock);
		T val = que.front();
		que.pop();
		return val;
	}
private:
	std::queue<T> que;
	std::mutex mut;
	std::condition_variable cond;
};

#endif
