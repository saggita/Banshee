/*
    Banshee and all code, documentation, and other materials contained
    therein are:

        Copyright 2013 Dmitry Kozlov
        All Rights Reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the software's owners nor the names of its
        contributors may be used to endorse or promote products derived from
        this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    (This is the Modified BSD License)
*/
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <iostream>

///< An implementation of a concurrent queue
///< which is providing the means to wait until 
///< elements are available or use non-blocking
///< try_pop method.
///<
template <typename T> class thread_safe_queue
{
public:
    thread_safe_queue(){}
    ~thread_safe_queue(){}

    // Push element: one of the threads is going to 
    // be woken up to process this if any
    void push(T const& t)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(t);
        cv_.notify_one();
    }

    // Push element: one of the threads is going to 
    // be woken up to process this if any
    void push(T&& t)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(t));
        cv_.notify_one();
    }

    // Wait until there are element to process.
    void wait_and_pop(T& t)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this](){return !queue_.empty();});
        t = queue_.front();
        queue_.pop();
    }

    // Try to pop element. Returns true if element has been popped, 
    // false if there are no element in the queue
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


///< Thread pool implementation which is using concurrency
///< available in the system
///<
template <typename RetType> class thread_pool
{
public:
    thread_pool() 
    {
        done_ = false;
        int num_threads = std::thread::hardware_concurrency();
        num_threads = num_threads == 0 ? 2 : num_threads;

#ifdef _DEBUG
        std::cout << num_threads << " hardware threads available\n";
#endif

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

    // Submit a new task into the pool. Future is returned in
    // order for caller to track the execution of the task
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
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }


    thread_safe_queue<std::packaged_task<RetType()> > work_queue_;
    std::atomic_bool done_;
    std::vector<std::thread> threads_;
};


#endif
