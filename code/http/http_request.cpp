#include "http_request.h"

const std::unordered_set<std::string> http_request::DEFAULT_HTML{
            "/index", "/register", "/login",
             "/welcome", "/video", "/picture", };

const std::unordered_map<std::string, int> http_request::DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login.html", 1},  };

void http_request::init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool http_request::is_keep_alive() const {
    // if(post_.count("Connection") == 1) {
    //     return post_.find("Connection")->second == "Keep-Alive" && version_ == "1.1";
    // }
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool http_request::parse(buffer& buff) {
    const char CRLF[] = "\r\n";
    if(buff.readable_bytes() <= 0) {
        return false;
    }
    while(buff.readable_bytes() && state_ != FINISH) {
        const char* line_end = std::search(buff.peek(), buff.begin_write_const(), CRLF, CRLF + 2);
        std::string line(buff.peek(), line_end);
        switch(state_)
        {
        case REQUEST_LINE:
            if(!parse_request_line_(line)) {
                return false;
            }
            parse_path_();
            break;    
        case HEADERS:
            parse_header_(line);
            if(buff.readable_bytes() <= 2) {
                 state_ = FINISH;
            }
            break;
        case BODY:
            parse_body_(line);
            break;
        default:
            break;
        }
        if(line_end == buff.begin_write()) {
            break; 
        }
        buff.retrieve_until(line_end + 2);

    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

void http_request::parse_path_() {
    if(path_ == "/") {
        path_ = "/index.html"; 
    }
    else {
        for(auto &item: DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

bool http_request::parse_request_line_(const std::string& line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if(regex_match(line, subMatch, patten)) {   
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void http_request::parse_header_(const std::string& line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    }
    else {
        state_ = BODY;
    }
}

void http_request::parse_body_(const std::string& line) {
    body_ = line;
    parse_post_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}


std::string http_request::get_post(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}


void http_request::parse_post_() {
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        parse_from_url_encoded_();
        if(DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag: %d", tag);
            if(tag == 0 || tag == 1) {
                bool log_in = tag == 1;
                if(verify_user_(post_["username"], post_["password"], log_in)) {
                    path_ = "/welcome.html";
                }
                else {
                    path_ = "/error.html";
                }
            }
        }
    }   

}

bool http_request::verify_user_(const std::string& username, const std::string& pwd, bool log_in) {
    if(username == "" || pwd == "")
        return false;
    LOG_INFO("To verify, username: %s, password: %s", username.c_str(), pwd.c_str());

    MYSQL* sql;
    sql_conn_RAII(&sql, sql_conn_pool::instance());
    assert(sql);

    // MYSQL_FIELD* fields = nullptr;
    MYSQL_RES* res = nullptr;

    std::string query = "ELECT username, password FROM user WHERE username=" + username +  "LIMIT 1";
    LOG_DEBUG("%s", query.c_str());

    if(mysql_query(sql, query.c_str()) != 0) {
        mysql_free_result(res);
        return false;
    }

    res = mysql_store_result(sql);
    // int num_fields = mysql_num_fields(res);
    // fields = mysql_fetch_fields(res);
    bool err = false;
    if(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("Mysql row: %s %s", row[0], row[1]);
        if(log_in) {
            if(pwd != row[1]) {
                LOG_DEBUG("password incorrect!");
                err = true;
            }
        }
        else {
            err = true;
        }
    }
    mysql_free_result(res);
    if(log_in == false && err == false) {
        // 注册
        LOG_DEBUG("register");
        query = "INSERT INTO user(username, password) VALUES('" + username + "', '" + pwd + "')";
        LOG_DEBUG("%s", query.c_str());
        if(mysql_query(sql, query.c_str()) != 0) {
            LOG_DEBUG("Insert Error!");
            err = true;
        }
    }
    sql_conn_pool::instance()->free_conn(sql);

    return err == false;

}

void http_request::parse_from_url_encoded_() {
    if(body_.size() == 0) { 
        return; 
    }

    std::string key, value;
    int n = body_.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
            case '=':
                key = body_.substr(j, i - j);
                j = i + 1;
                break;
            case '&':
                value = body_.substr(j, i - j);
                j = i + 1;
                post_[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
                break;
            default:
                break;
        }
    }
    assert(j <= i);
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

std::string http_request::path() const{
    return path_;
}

std::string& http_request::path(){
    return path_;
}
std::string http_request::method() const {
    return method_;
}

std::string http_request::version() const {
    return version_;
}