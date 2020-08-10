#ifndef SERVER_H
#define SERVER_H

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/timer_heap.h"
#include "../pool/thread_pool.h"
#include "../http/http_conn.h"

class server {
public:
    server(
        int port, int trig_mode, int timeout_ms, bool linger, int thread_num,
            bool open_log, int log_level, int log_que_size, const std::string& name);

    ~server();
    void start();

private:
    bool init_socket_(); 
    void init_event_mode_(int trig_mode);
    void add_client_(int fd, sockaddr_in addr);
  
    void deal_listen_();
    void deal_write_(http_conn* client);
    void deal_read_(http_conn* client);

    void send_error_(int fd, const char*info);
    void extent_time_(http_conn* client);
    void close_conn_(http_conn* client);

    void on_read_(http_conn* client);
    void on_write_(http_conn* client);
    
    static const int MAX_FD = 65536;

    static int set_fd_nonblock(int fd);

    int port_;
    bool linger_;
    int timeout_ms_;  /* 毫秒MS */
    bool is_closed_;
    int listen_fd_;
    char* src_dir_;
    
    uint32_t listen_event_;
    uint32_t conn_event_;
    int* pipe_fd_;
    size_t total_cnt_;
    char name_[10];
    std::unique_ptr<timer_heap> timer_;
    std::unique_ptr<thread_pool> thread_pool_;
    std::unique_ptr<epoller> epoller_;
    std::unordered_map<int, http_conn> users_;
};


#endif