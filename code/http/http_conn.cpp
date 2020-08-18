#include "http_conn.h"

bool http_conn::ET;
std::atomic<int> http_conn::user_cnt;
const char* http_conn::src_dir;


http_conn::http_conn() { 
    fd_ = -1;
    addr_ = { 0 };
    is_closed_ = true;
};

http_conn::~http_conn() { 
    close_conn(); 
};

void http_conn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    user_cnt++;
    addr_ = addr;
    fd_ = fd;
    reset();
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, get_ip(), get_port(), (int)user_cnt);
}

void http_conn::reset() {
    write_buff_.retrieve_all();
    read_buff_.retrieve_all();
    is_closed_ = false;
    request_.init();
}

void http_conn::close_conn() {
    response_.unmap_file();
    if(is_closed_ == false){
        is_closed_ = true; 
        user_cnt--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, get_ip(), get_port(), (int)user_cnt);
    }
}

int http_conn::get_fd() const {
    return fd_;
};

struct sockaddr_in http_conn::get_addr() const {
    return addr_;
}

const char* http_conn::get_ip() const {
    return inet_ntoa(addr_.sin_addr);
}

int http_conn::get_port() const {
    return ntohs(addr_.sin_port);
}

ssize_t http_conn::read(int* save_errno) {
    ssize_t len = -1;
    ssize_t total_len = 0;
    do {
        len = read_buff_.read_fd(fd_, save_errno);
        LOG_DEBUG("w pos: %d, len: %d, errno: %d", read_buff_.readable_bytes(), len, *save_errno);
        if (len <= 0) {
            break;
        }
        total_len += len;
    } while (ET);
    return total_len;
}

ssize_t http_conn::write(int* save_errno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iov_cnt_);
        if(len <= 0) {
            *save_errno = errno;
            break;
        }
        /* 传输结束 */
        if(iov_[0].iov_len + iov_[1].iov_len == 0) {
            break; 
        } 
        else if(static_cast<size_t>(len) > iov_[0].iov_len) {       // 0写完
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len) {
                write_buff_.retrieve_all();
                iov_[0].iov_len = 0;
            }
        }
        else {      
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            write_buff_.retrieve(len);
        }
    } while(ET || bytes_to_write() > 10240);
    return len;
}

void http_conn::process() {
    if(request_.parse(read_buff_)) {
        LOG_DEBUG("%s", request_.path().c_str());
        response_.init(src_dir, request_.path(), request_.is_keep_alive(), 200);
    } 
    else {
        response_.init(src_dir, request_.path(), false, 400);
    }
    response_.make_response(write_buff_);
    /* 响应头 */
    iov_[0].iov_base = const_cast<char*>(write_buff_.peek());
    iov_[0].iov_len = write_buff_.readable_bytes();
    iov_cnt_ = 1;

    /* 响应文件 */
    if(response_.file_len() > 0 && response_.file()) {
        iov_[1].iov_base = response_.file();
        iov_[1].iov_len = response_.file_len();
        iov_cnt_ = 2;
    } 
    LOG_DEBUG("filesize:%d, %d  to %d", response_.file_len() , iov_cnt_, bytes_to_write());
}
