/*
 * @Author       : mark
 * @Date         : 2020-06-17
 * @copyleft Apache 2.0
 */ 

#include "server.h"

using namespace std;

server::server(
            int port, int trig_mode, int timeout_ms, bool opt_linger,  int thread_num,
            int sql_port, const char* sql_user, const char* sql_pwd, const char* db_name, int sql_conn_num,
            bool open_log, int log_level, int log_que_size, const std::string& name):
            port_(port), linger_(opt_linger), timeout_ms_(timeout_ms), is_closed_(false), 
            timer_(new timer_heap()), thread_pool_(new thread_pool(thread_num)), epoller_(new epoller()) {
    strcpy(name_, name.c_str());
    src_dir_ = getcwd(nullptr, 256);
    total_cnt_ = 0;
    assert(src_dir_);
    strncat(src_dir_, "/resources", 16);
    http_conn::user_cnt = 0;
    http_conn::src_dir = src_dir_;
    
    sql_conn_pool::instance()->init("localhost", sql_port, sql_user, sql_pwd, db_name, sql_conn_num);

    init_event_mode_(trig_mode);
    if(!init_socket_()) { 
        is_closed_ = true;
    }
    if(open_log) { 
        Log::instance()->init(log_level, "./log", ".log", log_que_size); 
        if(is_closed_) { 
            LOG_ERROR("%s: ========== Server init error!==========", name_);
        }
        else {
            LOG_INFO("%s: ========== Server init ==========", name_);
            LOG_INFO("%s: Port:%d, Linger: %s", name_, port_, opt_linger? "true":"false");
            LOG_INFO("%s: Listen Mode: %s, Connection Mode: %s", name_, 
                            (listen_event_ & EPOLLET ? "ET": "LT"), 
                            (conn_event_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("%s: LogSys level: %d", name_, log_level);
            LOG_INFO("%s: src_dir: %s", name_, http_conn::src_dir);
            LOG_INFO("%s: ThreadPool num: %d, SQLPool num :%d", name_, thread_num, sql_conn_num);
        }
    }
}

server::~server() {
    close(listen_fd_);
    is_closed_ = true;
    free(src_dir_);
}

void server::init_event_mode_(int trig_mode) {
    listen_event_ = EPOLLRDHUP;
    conn_event_ = EPOLLONESHOT | EPOLLRDHUP; 
    switch (trig_mode)
    {
        case 0:
            break;
        case 1:
            conn_event_ |= EPOLLET;
            break;
        case 2:
            listen_event_ |= EPOLLET;
            break;
        case 3:
            listen_event_ |= EPOLLET;
            conn_event_ |= EPOLLET;
            break;
        default:
            listen_event_ |= EPOLLET;
            conn_event_ |= EPOLLET;
            break;
    }
    http_conn::ET = (conn_event_ & EPOLLET);
}

void server::start() {
    int time_ms = -1;  /* epoll wait timeout == -1 无事件将阻塞 */
    if(!is_closed_) { 
        LOG_INFO("%s: ========== Server start ==========", name_); 
    }
    while(!is_closed_) {
        if(timeout_ms_ > 0) {
            time_ms = timer_->get_next_tick();
        }
        int event_cnt = epoller_->wait(time_ms);
        for(int i = 0; i < event_cnt; i++) {
            /* 处理事件 */
            int fd = epoller_->get_event_fd(i);
            uint32_t events = epoller_->get_events(i);
            if(fd == listen_fd_) {
                deal_listen_();
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {      // 关闭连接
                assert(users_.count(fd) > 0);
                close_conn_(&users_[fd]);
            }
            else if(events & EPOLLIN) {             // 读
                assert(users_.count(fd) > 0);
                deal_read_(&users_[fd]);
            }
            else if(events & EPOLLOUT) {            // 写
                assert(users_.count(fd) > 0);
                deal_write_(&users_[fd]);
            } else {
                LOG_ERROR("%s: Unexpected event", name_);
            }
        }
    }
}

void server::send_error_(int fd, const char* info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
        LOG_WARN("%s: send error to client[%d] error!", name_, fd);
    }
    close(fd);
}

void server::close_conn_(http_conn* client) {
    assert(client);
    LOG_INFO("%s: Client[%d] quit!", name_, client->get_fd());
    epoller_->del_fd(client->get_fd());
    client->close_conn();
}

void server::add_client_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    total_cnt_ += 1;
    users_[fd].init(fd, addr);
    if(timeout_ms_ > 0) {
        timer_->add_timer(fd, timeout_ms_, std::bind(&server::close_conn_, this, &users_[fd]));
    }
    epoller_->add_fd(fd, EPOLLIN | conn_event_);
    set_fd_nonblock(fd);
    LOG_INFO("%s: Client[%d] in! total connection cnt: %d", name_, fd, total_cnt_);
}

void server::deal_listen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listen_fd_, (struct sockaddr *)&addr, &len);
        if(fd <= 0) { 
            break;
        }
        else if(http_conn::user_cnt >= MAX_FD) {
            send_error_(fd, "Server busy!");
            LOG_WARN("%s: Clients is full!", name_);
            return;
        }
        add_client_(fd, addr);
    } while(listen_event_ & EPOLLET);
}

void server::deal_read_(http_conn* client) {
    assert(client);
    extent_time_(client);
    thread_pool_->add_task(std::bind(&server::on_read_, this, client));
}

void server::deal_write_(http_conn* client) {
    assert(client);
    extent_time_(client);
    thread_pool_->add_task(std::bind(&server::on_write_, this, client));
}

void server::extent_time_(http_conn* client) {
    assert(client);
    if(timeout_ms_ > 0) { 
        timer_->adjust(client->get_fd(), timeout_ms_); 
    }
}

void server::on_read_(http_conn* client) {
    assert(client);
    int ret = -1;
    int read_errno = 0;
    ret = client->read(&read_errno);
    if(ret <= 0 && read_errno != EAGAIN) {
        close_conn_(client);
        return;
    }
    client->process();
    epoller_->mod_fd(client->get_fd(), conn_event_ | EPOLLOUT);
}
 
void server::on_write_(http_conn* client) {
    assert(client);
    int ret = -1;
    int write_errno = 0;
    ret = client->write(&write_errno);
    if(client->bytes_to_write() == 0) {
        /* 传输完成 */
        if(client->is_keep_alive()) {
            LOG_DEBUG("%s: keepAlive!", name_);
            client->reset();
            epoller_->mod_fd(client->get_fd(), conn_event_ | EPOLLIN);
            return;
        }
    }
    else if(ret < 0) {
        if(write_errno == EAGAIN) {
            /* 继续传输 */
            epoller_->mod_fd(client->get_fd(), conn_event_ | EPOLLOUT);
            return;
        }
    }
    close_conn_(client);
}

/* Create listenFd */   
bool server::init_socket_() {        
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 && port_ < 1024) {
        LOG_ERROR("%s: Port:%d error!", name_, port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    struct linger opt_linger = {0}; 
    if(linger_) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        opt_linger.l_onoff = 1;
        opt_linger.l_linger = 1;
    }

    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd_ < 0) {
        LOG_ERROR("%s: Create socket error!", name_, port_);
        return false;
    }

    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &opt_linger, sizeof(opt_linger));
    if(ret < 0) {
        close(listen_fd_);
        LOG_ERROR("%s: Init linger error!", port_);
        return false;
    }
    
    int opt_val = 1;
    /* 端口复用 */
    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt_val, sizeof(int));
    if(ret == -1) {
        LOG_ERROR("%s: set socket setsockopt error !", name_);
        close(listen_fd_);
        return false;
    }

    ret = bind(listen_fd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("%s: Bind Port:%d error!", name_, port_);
        close(listen_fd_);
        return false;
    }

    ret = listen(listen_fd_, 6);
    if(ret < 0) {
        LOG_ERROR("%s: Listen port:%d error!", name_, port_);
        close(listen_fd_);
        return false;
    }
    ret = epoller_->add_fd(listen_fd_,  listen_event_ | EPOLLIN);
    if(ret == 0) {
        LOG_ERROR("%s: Add listen error!", name_);
        close(listen_fd_);
        return false;
    }
    set_fd_nonblock(listen_fd_);
    LOG_INFO("%s: Server port:%d", name_, port_);
    return true;
}

int server::set_fd_nonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}


