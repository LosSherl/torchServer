#include "epoller.h"

epoller::epoller(int max_event):epoll_fd_(epoll_create(512)), events_(max_event){
    assert(epoll_fd_ >= 0 && events_.size() > 0);
}

epoller::~epoller() {
    close(epoll_fd_);
}

bool epoller::add_fd(int fd, uint32_t events) {
    if(fd < 0) 
        return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
}

bool epoller::mod_fd(int fd, uint32_t events) {
    if(fd < 0) 
        return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
}

bool epoller::del_fd(int fd) {
    if(fd < 0) return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &ev);
}

int epoller::wait(int timeout_ms) {
    return epoll_wait(epoll_fd_, &events_[0], static_cast<int>(events_.size()), timeout_ms);
}

int epoller::get_event_fd(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].data.fd;
}

uint32_t epoller::get_events(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].events;
}