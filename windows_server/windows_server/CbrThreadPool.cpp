#include "CbrThreadPool.h"
#ifdef _DEBUG
#define DEBUG_
#endif

// 默认构造
CbrThreadPool::CbrThreadPool(): min_(1), max_(4) {

    std::cout << "创建默认线程池，min 1 - max 4" << std::endl;
    current_waiting_tasks_number_.store(0);
    current_threads_number_.store(0);
    current_idle_thread_.store(0);
    current_tasks_number_.store(0);
    terminal_.store(false);

    Init(1, 4);
}

// 创建指定数量线程池
CbrThreadPool::CbrThreadPool(const int min, const int max)
    : min_(min), max_(max) {

    std::cout << "创建指定数量线程池" << std::endl;
    Init(min, max);
}

// 创建超时等待线程池
CbrThreadPool::CbrThreadPool(const int min, const int max, const int timeout)
    : min_(min), max_(max) {
    std::cout << "创建超时等待线程池" << std::endl;
    Init(min, max, timeout);
}

// 析构
CbrThreadPool::~CbrThreadPool() {
    if (!stop_.load())
        Stop();

    std::cout << "销毁" << std::endl;
}

// 终止所有线程，清除所有队列
void CbrThreadPool::Stop() {

    std::cout << "终止线程" << std::endl;
    stop_.store(true);

    condition_.notify_all();
    for (auto& element : threads_pool_) {
        if (element.joinable())
            element.join();
    }

    // while (!this->tasks_.empty()) {
    //  // 清除队列
    //  const auto clear_task = &tasks_.front();
    //  this->tasks_.pop();
    //  delete clear_task;
    // }

}


// 初始化线程
void CbrThreadPool::Init(const int min, const int max) {
    // current_threads_number_.store(min);
    stop_.store(false);
    for (auto i = 0; i < min; ++i) {
        std::unique_lock<std::mutex> lock{this->thread_lock_};
        threads_pool_.emplace_back([this]() {
            CreateThread();
        });

    }
    std::thread watch(&CbrThreadPool::DynamicAdjustThreadNumber, this, min, max);
    watch.detach();
}

// 初始化线程
void CbrThreadPool::Init(const int min, const int max,
                         const int timeout) {
    // current_threads_number_.store(min);
    stop_.store(false);
    for (auto i = 0; i < min; ++i) {
        std::unique_lock<std::mutex> lock{this->thread_lock_};
        threads_pool_.emplace_back([this, timeout]() {
            CreateThread(timeout);
        });
    }
    std::thread watch(&CbrThreadPool::DynamicAdjustThreadNumber, this, min, max);
    watch.detach();
}



// 动态增减线程数
void CbrThreadPool::DynamicAdjustThreadNumber(const int min = 2, const int max = 4) {
    auto start = GetTickCount();
    auto stop = GetTickCount();
    while (true) {
        if (this->stop_)
            return;
        if ((this->current_threads_number_.load() < max) &&
                (this->current_tasks_number_.load() > min) &&
                (this->current_threads_number_.load() <
                 current_tasks_number_.load())) {
            std::unique_lock<std::mutex> lock{this->thread_lock_};
            threads_pool_.emplace_back([this]() {
                this->CreateThread();
                // lock.unlock();
                // lock.lock();
            });
        }
        if (this->current_tasks_number_.load() < current_threads_number_.load()
                && this->current_threads_number_.load() > min) {
            std::lock_guard<std::mutex> lock{task_lock_};
            this->terminal_.store(true);
            condition_.notify_one();
            // this->terminal_.store(false);
        }

        // 等待任务不为空 而且 空闲进程有剩则唤醒
        if (current_waiting_tasks_number_.load() > 0 && current_idle_thread_.load() > 0) {
            std::lock_guard<std::mutex> lock{task_lock_};
            this->condition_.notify_one();
        }

        // 有空转等待的线程，而且空转数比等待数要多就销毁
        if (current_idle_thread_.load() >
                current_waiting_tasks_number_.load()) {
            stop = GetTickCount();
            if ((stop - start) > 1000) {
                std::lock_guard<std::mutex> lock{task_lock_};
                this->terminal_.store(true);
                condition_.notify_one();
                start = GetTickCount();
            }
        }

#ifdef DEBUG
        std::cout << "current idle thread number："
                  << current_idle_thread_.load()
                  << "\tcurrent thread number："
                  << current_threads_number_.load() << std::endl;
        std::cout << "current tasks number：" << current_tasks_number_.load()
                  << "\tcurrent waiting tasks number："
                  << current_waiting_tasks_number_.load() << std::endl;
#endif

    }

}


// 抽象出来的线程创建函数
void CbrThreadPool::CreateThread() {
    ++current_threads_number_;
    ++current_idle_thread_;
#ifdef DEBUG
    std::cout << "创建线程" << std::endl;
#endif
    while (true) {
        std::function<void()> execute_task;

        // 等待锁、以及判断是否有终止标志、以及任务队列是否为空

        std::unique_lock<std::mutex> lock{this->task_lock_};
        // ++current_waiting_tasks_number_;
        if (this->stop_.load()) {
#ifdef DEBUG
            std::cout << "检测到停止信号" << std::endl;
#endif
            break;
            //如果任务队列为空，则进去等待
        }
        if (tasks_.empty()) {

            condition_.wait(lock, [this]() {
                return this->stop_.load() || !this->tasks_.empty();
            });
#ifdef DEBUG
            std::cout << "收到通知" << std::endl;
#endif

            if (this->current_threads_number_.load() > min_) {
                --this->current_idle_thread_;
                --this->current_threads_number_;
                this->terminal_.store(false);
#ifdef DEBUG
                std::cout << "销毁多余线程" << std::endl;
                std::cout << "current idle thread number："
                          << current_idle_thread_.load()
                          << "\tcurrent thread number："
                          << current_threads_number_.load() << std::endl;
#endif
                return;
            }

            // // 配合监控线程数的线程进行线程数控制
            //
            // if (this->terminal_.load()) {
            //     --this->current_idle_thread_;
            //     --this->current_threads_number_;
            //    std::cout << "销毁多余线程" <<std::endl;
            //    std::cout << "current idle thread number："
            //          << current_idle_thread_.load()
            //          << "\tcurrent thread number："
            //          << current_threads_number_.load() <<
            //         std::endl;
            //     this->terminal_.store(false);
            //     // this->threads_pool_.
            //     return;
            // }

        } else {
            // --current_waiting_tasks_number_;
            // 停止标志，以及任务队列空则销毁线程
            // TODO: 考虑添加一个最少线程判断在下方
            if (this->stop_.load() && this->tasks_.empty())
                return;

            execute_task = move(this->tasks_.front());
            this->tasks_.pop();
            --this->current_idle_thread_;


            lock.unlock();
            execute_task(); //running task
            lock.lock();
            --current_tasks_number_;

            ++this->current_idle_thread_;
        }


    }
}


// 抽象出来的线程创建函数
void CbrThreadPool::CreateThread(const int timeout) {
    ++current_threads_number_;
    ++current_idle_thread_;
#ifdef DEBUG
    std::cout << "创建线程" << std::endl;
#endif
    while (true) {
        std::function<void()> execute_task;

        std::unique_lock<std::mutex> lock{this->task_lock_};

        ++current_waiting_tasks_number_;
        if (this->stop_.load()) {
#ifdef DEBUG
            std::cout << "检测到停止信号" << std::endl;
#endif
            break;
            // 等待锁、以及判断是否有终止标志、以及任务队列是否为空

            //如果任务队列为空，则进去等待
        }
        if (tasks_.empty()) {
            condition_.wait_for(lock, std::chrono::seconds(timeout), [this]() {
                return this->stop_.load() || !this->tasks_.empty();
            });
#ifdef DEBUG
            std::cout << "收到通知" << std::endl;
#endif
            if (this->current_threads_number_.load() > min_) {
                --this->current_idle_thread_;
                --this->current_threads_number_;
                this->terminal_.store(false);
#ifdef DEBUG
                std::cout << "销毁多余线程" << std::endl;
                std::cout << "current idle thread number：" << current_idle_thread_.load()
                          << "\tcurrent thread number：" << current_threads_number_.load()
                          << std::endl;
#endif
                return;
            }

            // 配合监控线程数的线程进行线程数控制

            // if (this->terminal_.load()) {
            //     --this->current_idle_thread_;
            //     --this->current_threads_number_;
            //    std::cout << "销毁多余线程" <<std::endl;
            //     this->terminal_.store(false);
            //     // this->threads_pool_.
            //     return;
            // }

        } else {
            --current_waiting_tasks_number_;
            // 停止标志，以及任务队列空则销毁线程
            // TODO: 考虑添加一个最少线程判断在下方
            if (this->stop_.load() && this->tasks_.empty()) return;

            execute_task = move(this->tasks_.front());
            this->tasks_.pop();
            --this->current_idle_thread_;
#ifdef DEBUG
            std::cout << "current idle thread number：" << current_idle_thread_.load()
                      << "\tcurrent thread number：" << current_threads_number_.load()
                      << std::endl;
            std::cout << "current tasks number：" << current_tasks_number_.load()
                      << "\tcurrent waiting tasks number："
                      << current_waiting_tasks_number_.load() << std::endl;
#endif

            lock.unlock();
            execute_task(); // running task
            lock.lock();
            --current_tasks_number_;

            ++this->current_idle_thread_;
        }
    }
}
