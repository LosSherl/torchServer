#ifndef BLOCK_DEQUE_H
#define BLOCK_DEQUE_H

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>

template <typename T>
class block_deque {
public:
    explicit block_deque(size_t max_capacity = 1000);
    
    ~block_deque();

    void clear();
    
    bool empty();

    bool full();

    void close();

    size_t size();

    size_t capacity();

    T front();

    T back();

    void push_back(const T& item);

    void push_front(const T& item);

    bool pop_front(T& item);

    bool pop_front(T& item, int timeout);

    void flush();

private:
    std::deque<T> deq_;

    size_t capacity_;

    std::mutex mtx_;

    bool is_closed_;

    std::condition_variable cond_consumer_;

    std::condition_variable cond_producer_;
};

template <typename T>
block_deque<T>::block_deque(size_t max_capacity) : capacity_(max_capacity) {
    assert(max_capacity > 0);
    is_closed_ = false;
}

template <typename T>
block_deque<T>::~block_deque() {
    close();
}

template <typename T>
void block_deque<T>::close() {
    {
        std::lock_guard<std::mutex> locker(mtx_);
        deq_.clear();
        is_closed_ = true;
    }
    cond_producer_.notify_all();
    cond_consumer_.notify_all();
}

template <typename T>
void block_deque<T>::clear() {
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
}

template <typename T>
void block_deque<T>::flush() {
    cond_consumer_.notify_one();
}

template <typename T>
T block_deque<T>::front() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
}

template <typename T>
T block_deque<T>::back() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}

template <typename T>
size_t block_deque<T>::capacity() {
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template <typename T>
void block_deque<T>::push_back(const T& item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.size() >= capacity_) {
        cond_producer_.wait(locker);
    }
    deq_.push_back(item);
    cond_consumer_.notify_one();
}

template <typename T>
void block_deque<T>::push_front(const T& item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.size() >= capacity_) {
        cond_producer_.wait(locker);
    }
    deq_.push_front(item);
    cond_consumer_.notify_one();
}

template<typename T>
bool block_deque<T>::empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}

template<typename T>
bool block_deque<T>::full(){
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}

template<typename T>
bool block_deque<T>::pop_front(T& item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()) {
        cond_consumer_.wait(locker);
        if(is_closed_) {
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    cond_producer_.notify_one();
    return true;
}

template<typename T>
bool block_deque<T>::pop_front(T& item, int timeout) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()) {
        if(cond_consumer_.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout) {
            return false;
        }
        if(is_closed_) {
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    cond_producer_.notify_one();
    return true;
}

#endif