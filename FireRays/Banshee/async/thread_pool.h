#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <iostream>

template <typename T> class thread_safe_queue
{
public:
	thread_safe_queue(){}
	~thread_safe_queue(){}

	void push(T const& t)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		queue_.push(t);
		cv_.notify_one();
	}

	void push(T&& t)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		queue_.push(std::move(t));
		cv_.notify_one();
	}
	
	void wait_and_pop(T& t)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		cv_.wait(lock, [this](){return !queue_.empty()});
		t = queue_.front();
		queue_.pop();
	}

	bool try_pop(T& t)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (queue_.empty())
			return false;
		t = std::move(queue_.front());
		queue_.pop();
		return true;
	}

	

private:
	std::mutex mutex_;
	std::condition_variable cv_;
	std::queue<T> queue_;
};



template <typename RetType> class thread_pool
{
public:

	thread_pool() 
	{
		done_ = false;
		int num_threads = std::thread::hardware_concurrency();
		num_threads = num_threads == 0 ? 2 : num_threads;
		
		std::cout << num_threads << " hardware threads available\n";
		
		for (int i=0; i < num_threads; ++i)
		{
			threads_.push_back(std::thread(&thread_pool::run_loop, this));
		}
	}

	~thread_pool()
	{
		done_ = true;
		std::for_each(threads_.begin(), threads_.end(), std::mem_fun_ref(&std::thread::join));
	}

	std::future<RetType> submit(std::function<RetType()>&& f)
	{
		std::packaged_task<RetType()> task(std::move(f));
		auto future = task.get_future();
		work_queue_.push(std::move(task));
		return future;
	}

private:
	void run_loop()
	{
		std::packaged_task<RetType()> f;
		while(!done_)
		{
			if (work_queue_.try_pop(f))
			{
				f();
			}
			else
			{
				std::this_thread::yield();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
	}


	thread_safe_queue<std::packaged_task<RetType()> > work_queue_;
	std::atomic_bool done_;
	std::vector<std::thread> threads_;
};


#endif
