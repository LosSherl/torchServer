#ifndef SQL_CONN_POOL_H
#define SQL_CONN_POOL_H

#include <mysql/mysql.h>


class sql_conn_pool {
public:
    static sql_conn_pool* instance();
    
    MYSQL* get_conn();
    void free_conn(MYSQL* conn);
    int get_free_conn_count();

    void init(const char* host, int port, const char* username,
        const char* pwd, const char* db_name, int pool_size);

    void close_pool();

private:
    sql_conn_pool();
    ~sql_conn_pool();
}


#endif