#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>     

#include "../buffer/buffer.h"
#include "../pool/sql_conn_pool.h"
#include "../pool/sql_conn_RAII.h"
#include "../log/log.h"

class http_request {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,        
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    
    http_request() { init(); }
    ~http_request() = default;

    void init();
    bool parse(buffer& buff);

    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string get_post(const std::string& key) const;
    // std::string get_Post(const char* key);

    bool is_keep_alive() const;

private:
    bool parse_request_line_(const std::string& line);
    void parse_header_(const std::string& line);
    void parse_body_(const std::string& line);

    void parse_path_();
    void parse_post_();
    void parse_from_url_encoded_();

    bool verify_user_(const std::string& username, const std::string& pwd, bool log_in);

    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
};


#endif