#ifndef TIMER_HEAP_H
#define TIMER_HEAP_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>

#include "../log/log.h"

typedef std::function<void()> call_back_func;
typedef std::chrono::high_resolution_clock hs_clock;
typedef std::chrono::milliseconds MS;
typedef hs_clock::time_point time_stamp;

struct timer {
    int id;
    time_stamp expires;
    call_back_func cb;
    bool operator<(const timer& t) {
        return expires < t.expires;
    }
};

class timer_heap {
public:
    timer_heap() { heap_.reserve(64); }

    ~timer_heap() { clear(); }
    
    void adjust(int id, int new_expires_time);

    void add_timer(int id, int timeout, const call_back_func& cb);

    void rm_timer(int id);

    void clear();

    void tick();

    void pop();

    int get_next_tick();

private:
    void del_(size_t i);
    
    void sift_up_(size_t i);

    bool sift_down_(size_t index, size_t n);

    void swap_node_(size_t i, size_t j);

    std::vector<timer> heap_;

    std::unordered_map<int, size_t> ref_;
};

#endif 