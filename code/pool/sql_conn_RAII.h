#ifndef SQL_CONN_RAII_H
#define SQL_CONN_RAII_H

#include "sql_conn_pool.h"

class sql_conn_RAII {
public:
    sql_conn_RAII(MYSQL** sql, sql_conn_pool* conn_pool) {
        assert(conn_pool);
        *sql = conn_pool->get_conn();
        sql_ = *sql;
        conn_pool_ = conn_pool;
    }

    ~sql_conn_RAII() {
        if(sql_)
            conn_pool_->free_conn(sql_);
    }
private:
    MYSQL* sql_;
    sql_conn_pool* conn_pool_;
};

#endif