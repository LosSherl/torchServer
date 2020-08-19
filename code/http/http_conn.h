#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      
#include <stdio.h>

#include "../log/log.h"
#include "../buffer/buffer.h"
#include "http_request.h"
#include "http_response.h"

class http_conn {
public:
    http_conn();
    ~http_conn();

    void init(int sock_fd, const sockaddr_in& addr);
    void reset();
    ssize_t read(int* save_errno);
    ssize_t write(int* save_errno);

    void close_conn();

    int get_fd() const;
    int get_port() const;
    const char* get_ip() const;
    sockaddr_in get_addr() const;
    
    bool process();
    int bytes_to_write() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    bool is_keep_alive() const {
        return request_.is_keep_alive();
    }

    static bool ET;
    static const char* src_dir;
    static std::atomic<int> user_cnt;
    
private:
   
    int fd_;
    struct sockaddr_in addr_;

    bool is_closed_;
    
    int iov_cnt_;
    struct iovec iov_[2];
    
    buffer read_buff_; // 读缓冲区
    buffer write_buff_; // 写缓冲区

    http_request request_;
    http_response response_;
};


#endif