#include "sql_conn_pool.h"

sql_conn_pool::~sql_conn_pool() {
    close_pool();
}

sql_conn_pool* sql_conn_pool::instance() {
    static sql_conn_pool pool_;
    return &pool_;
}

void sql_conn_pool::init(const char* host, int port, const char* user, const char* pwd,
        const char* db_name, int conn_size = 10) {
    assert(conn_size > 0);
    for(int i = 0; i < conn_size; i++) {
        MYSQL* sql = nullptr;
        sql = mysql_init(sql);
        if(!sql) {
            LOG_ERROR("Mysql init error");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host, user, pwd, db_name, port, nullptr, 0);

        if(!sql) {
            LOG_ERROR("Mysql connect error");
        }
        conn_queue_.push(sql);
    }
    MAX_CONN_ = conn_size;
}

MYSQL* sql_conn_pool::get_conn() {
    MYSQL* sql = nullptr;
    std::unique_lock<std::mutex> locker(mtx_);
    while(conn_queue_.empty()) {    
        cond_.wait(locker);
    }
    sql = conn_queue_.front();
    conn_queue_.pop();
    return sql;
}

void sql_conn_pool::free_conn(MYSQL* sql) {
    assert(sql);
    std::lock_guard<std::mutex> locker(mtx_);
    conn_queue_.push(sql);
    cond_.notify_one();
}

void sql_conn_pool::close_pool() {
    std::lock_guard<std::mutex> locker(mtx_);
    while(!conn_queue_.empty()) {
        MYSQL* conn = conn_queue_.front();
        conn_queue_.pop();
        mysql_close(conn);
    }
    mysql_library_end();
}

int sql_conn_pool::get_free_conn_count() {
    std::lock_guard<std::mutex> locker(mtx_);
    return conn_queue_.size();
}

