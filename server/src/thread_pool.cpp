#include "thread_pool.h"

#include <iostream>

thread_pool::_thread::_thread(thread_pool& parent): parent{parent}
{
	// this cant be done in initializer list, otherwise it causes pthread_lock_mutex() to throw EINVAL
	// (mutex is probably not initialized yet, could be because it initializes after hndl)
	hndl = std::thread(&thread_pool::_thread::loop, this);
}
void thread_pool::_thread::loop()
{
	std::unique_lock lock(mut);
	for(;;){
		cond.wait(lock, [this](){ return run || finished; });
		if(finished)
			break;

		run_func();
		run = false;
		parent.release(this);
	}
}

thread_pool::thread_pool(size_t init_size): pool_size{init_size}
{
	for(size_t i = 0; i < init_size; ++i)
		threads.push(new thread_pool::_thread(*this));
}
thread_pool::~thread_pool()
{
	// Wait until all threads are available
	std::unique_lock lock(mut);
	cond.wait(lock, [this](){ return threads.size() == pool_size; });

	while(threads.size()){
		thread_pool::_thread* thr = threads.front();
		threads.pop();
		thr->finished = true;
		thr->cond.notify_one();
		thr->hndl.join();
		delete thr;
	}
}

void thread_pool::execute(std::function<void()> func, bool blocking)
{
	thread_pool::_thread* thr;
	{
		// Wait until a thread is available
		std::unique_lock lock(mut);
		std::cerr << "threads " << threads.size() << std::endl;
		if(!blocking && !threads.size())
			return;
		cond.wait(lock, [this](){ return threads.size(); });
		thr = threads.front();
		threads.pop();
	}

	thr->mut.lock();
	thr->run_func = func;
	thr->run = true;
	thr->cond.notify_one();
	thr->mut.unlock();
}
void thread_pool::release(thread_pool::_thread* thr)
{
	std::lock_guard lock(mut);
	threads.push(thr);
	cond.notify_one();
}
