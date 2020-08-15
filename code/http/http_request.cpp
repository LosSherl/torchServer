#include "http_request.h"

const std::string http_request::base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ""abcdefghijklmnopqrstuvwxyz""0123456789+/";

const std::unordered_set<std::string> http_request::DEFAULT_HTML{
            "/index", "/register", "/login",
             "/welcome", "/result"};

const std::unordered_map<std::string, int> http_request::DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login.html", 1},  };

void http_request::init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool http_request::is_keep_alive() const {
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
    LOG_DEBUG("%s", line.c_str());
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
        else if(path_ == "/result.html") {
            std::string img_str = std::move(base64_decode(post_["img_str"]));
            torch_model::instance()->classify(img_str);
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

    std::string query = "SELECT username, password FROM user WHERE username=" + username +  " LIMIT 1";
    LOG_DEBUG("%s", query.c_str());

    if(mysql_query(sql, query.c_str()) != 0) {
        mysql_free_result(res);
        return false;
    }

    res = mysql_store_result(sql);
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

    std::string key = "";
    std::string cur = "";
    size_t n = body_.size();
    size_t i = 0;

    for(; i < n; i++) {
        switch (body_[i]) {
            case '=':
                key = std::move(cur);
                cur = "";
                break;
            case '%':
                char ascii_val = convert_from_hex_(body_[i + 1]) * 16;
                ascii_val += convert_from_hex_(body_[i + 2]);
                i += 2;
                cur += ascii_val;
                break;
            case '&':
                post_[key] = cur;
                LOG_DEBUG("%s = %s", key.c_str(), cur.c_str());
                cur = "";
                break;
            default:
                cur += body_[i];
                break;
        }
    }
    if(post_.count(key) == 0 && cur.length() > 0) {
        post_[key] = cur;
    }
}

char http_request::convert_from_hex_(char c) {
    if(c >= 'a' && c <= 'z')
		return 10 + c - 'a';
	if(c >= 'A' && c <= 'Z')
		return 10 + c - 'A';
	return c - '0';
}


std::string http_request::base64_decode(const std::string& encoded_string) {
    size_t in_len = encoded_string.size();
    size_t i = 0;
    size_t j = 0;
    size_t in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }
    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;
        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }
    return ret;
}

std::string http_request::path() const {
    return path_;
}

std::string& http_request::path() {
    return path_;
}
std::string http_request::method() const {
    return method_;
}

std::string http_request::version() const {
    return version_;
}