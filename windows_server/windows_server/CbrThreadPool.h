#pragma once

#include <atomic>
#include <future>
#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <Windows.h>


//using namespace std;

class CbrThreadPool {
    public:
        CbrThreadPool();
        CbrThreadPool(int min, int max);
        CbrThreadPool(int min, int max, int timeout);
        ~CbrThreadPool();
        template <class F, class ... Args>
        auto PushTask(F&& f, Args&& ... args)->std::future<decltype(f(args...))>;
        //
        // template <class F>
        // auto PushTask(F&& f)->std::future<decltype(f(0))>;

        void Stop();

    private:
        void Init(int min, int max);
        void Init(int min, int max, int timeout);
        void DynamicAdjustThreadNumber(int min, int max);
        void CreateThread();
        void CreateThread(const int timeout);


    private:
        using task = std::function<void()>;
        const int min_;
        const int max_;
        std::mutex task_lock_;
        std::mutex thread_lock_;
        std::condition_variable condition_;
        std::atomic<int> current_idle_thread_;
        std::atomic<int> current_threads_number_;
        std::atomic<int> current_tasks_number_;
        std::atomic<int> current_waiting_tasks_number_;
        std::atomic<bool> stop_;
        std::atomic<bool> terminal_;
        std::vector<std::thread> threads_pool_;
        std::queue<task> tasks_;
};

template<class F, class... Args>
auto CbrThreadPool::PushTask(F&& f, Args&& ... args) -> std::future<decltype(f(args...))> {

    using ret_type = decltype(f(args...));
    auto pack_task = std::make_shared<std::packaged_task<ret_type()>>(
                         std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                     );

    std::lock_guard<std::mutex> lock{ task_lock_ };
    tasks_.emplace(
    [pack_task]() {
        (*pack_task)();
    }
    );
    ++current_tasks_number_;
    this->condition_.notify_one();
    return pack_task->get_future();

}