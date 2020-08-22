#include "timer_heap.h"

void timer_heap::swap_node_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
} 

void timer_heap::sift_up_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;
    while(j >= 0 && j < heap_.size()) {
        if(heap_[j] < heap_[i]) { 
            break; 
        }
        swap_node_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}


bool timer_heap::sift_down_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while(j < n) {
        if(j + 1 < n && heap_[j + 1] < heap_[j]) 
            j++;
        if(heap_[i] < heap_[j]) 
            break;
        swap_node_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void timer_heap::add_timer(int id, int timeout, const call_back_func& cb) {
    assert(id >= 0);
    size_t i;
    if(ref_.count(id) == 0) {
        /* 新节点：堆尾插入，调整堆 */
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, hs_clock::now() + MS(timeout), cb});
        sift_up_(i);
    } 
    else {
        /* 已有结点：调整堆 */
        i = ref_[id];
        heap_[i].expires = hs_clock::now() + MS(timeout);
        heap_[i].cb = cb;
        if(!sift_down_(i, heap_.size())) {
            sift_up_(i);
        }
    }
}

void timer_heap::rm_timer(int id) {
    /* 删除指定id结点，并触发回调函数 */
    if(heap_.empty() || ref_.count(id) == 0) {
        return;
    }
    size_t i = ref_[id];
    timer node = heap_[i];
    node.cb();
    del_(i);
}

void timer_heap::del_(size_t index) {
    /* 删除指定位置的结点 */
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    /* 将要删除的结点换到队尾，然后调整堆 */
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if(i < n) {
        swap_node_(i, n);
        if(!sift_down_(i, n)) {
            sift_up_(i);
        }
    }
    /* 队尾元素删除 */
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void timer_heap::adjust(int id, int timeout) {
    /* 调整指定id的结点 */
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expires = hs_clock::now() + MS(timeout);;
    sift_down_(ref_[id], heap_.size());
}

void timer_heap::tick() {
    /* 清除超时结点 */
    if(heap_.empty()) {
        return;
    }
    while(!heap_.empty()) {
        timer node = heap_.front();
        if(std::chrono::duration_cast<MS>(node.expires - hs_clock::now()).count() > 0) { 
            break; 
        }
        node.cb();
        pop();
    }
}

void timer_heap::pop() {
    assert(!heap_.empty());
    del_(0);
}

void timer_heap::clear() {
    ref_.clear();
    heap_.clear();
}

int timer_heap::get_next_tick() {
    tick();
    size_t res = -1;
    if(!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front().expires - hs_clock::now()).count();
        if(res < 0) { res = 0; }
    }
    return res;
}