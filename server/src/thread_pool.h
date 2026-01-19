#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <queue>

class thread_pool
{
public:
	thread_pool(size_t init_size = 8);
	~thread_pool();

	void execute(std::function<void()>);
private:
	struct _thread
	{
		thread_pool& parent;

		std::thread hndl;
		std::mutex mut;
		std::condition_variable cond;

		bool run = false, finished = false;
		std::function<void()> run_func;

		_thread(thread_pool&);
		void loop();
	};
	void release(_thread*);

	std::mutex mut;
	std::condition_variable cond;
	std::queue<_thread*> threads;
	size_t pool_size; // tracks total amount of threads for destructor
};

#endif
