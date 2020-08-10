#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <cassert>

class thread_pool {
public:
    explicit thread_pool(size_t core_cnt = 8): pool_(std::make_shared<Pool>()) {
            assert(core_cnt > 0);
            for(size_t i = 0; i < core_cnt; i++) {
                std::thread([pool = pool_] {    // 线程执行函数
                    std::unique_lock<std::mutex> locker(pool->mtx);
                    while(true) {
                        if(!pool->tasks.empty()) {
                            auto task = std::move(pool->tasks.front());
                            pool->tasks.pop();
                            locker.unlock();
                            task();
                            locker.lock();
                        } 
                        else if(pool->is_closed) 
                            break;
                        else 
                            pool->cond.wait(locker);
                    }
                }).detach();
            }
    }

    thread_pool() = default;

    thread_pool(thread_pool&&) = default;
    
    ~thread_pool() {
        if(static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->is_closed = true;
            }
            pool_->cond.notify_all();
        }
    }

    template <typename F>
    void add_task(F&& task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F>(task));
        }
        pool_->cond.notify_one();
    }

private:
    struct Pool {
        std::mutex mtx;
        std::condition_variable cond;
        bool is_closed;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool_;
};


#endif 