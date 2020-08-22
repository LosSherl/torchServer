#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>

class epoller {
public:
    explicit epoller(int maxEvent = 1024);

    ~epoller();

    bool add_fd(int fd, uint32_t events);

    bool mod_fd(int fd, uint32_t events);

    bool del_fd(int fd);

    int wait(int timeout_ms = -1);

    int get_event_fd(size_t i) const;

    uint32_t get_events(size_t i) const;
        
private:
    int epoll_fd_;

    std::vector<struct epoll_event> events_;    
};

#endif
