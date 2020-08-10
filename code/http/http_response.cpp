#include "http_response.h"


const std::unordered_map<std::string, std::string> http_response::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> http_response::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const std::unordered_map<int, std::string> http_response::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

http_response::http_response() {
    code_ = -1;
    path_ = src_dir_ = "";
    is_keep_alive_ = false;
    mm_file_ = nullptr; 
    mm_file_stat_ = { 0 };
};

http_response::~http_response() {
    unmap_file();
}

void http_response::init(const std::string& src_dir, std::string& path, bool is_keep_alive, int code){
    assert(src_dir != "" && path != "");
    if(mm_file_) { 
        unmap_file(); 
    }
    code_ = code;
    is_keep_alive_ = is_keep_alive;
    path_ = path;
    src_dir_ = src_dir;
    mm_file_ = nullptr; 
    mm_file_stat_ = { 0 };
}

void http_response::make_response(buffer& buff) {
    /* 判断请求的资源文件 */
    if(stat((src_dir_ + path_).data(), &mm_file_stat_) < 0 || S_ISDIR(mm_file_stat_.st_mode)) {
        code_ = 404;
    }
    else if(!(mm_file_stat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if(code_ == -1) { 
        code_ = 200; 
    }
    error_html_();
    add_state_line_(buff);
    add_header_(buff);
    add_content_(buff);
}

char* http_response::file() {
    return mm_file_;
}

size_t http_response::file_len() const {
    return mm_file_stat_.st_size;
}

void http_response::error_html_() {
    if(CODE_PATH.count(code_) == 1) {
        path_ = CODE_PATH.find(code_)->second;
        stat((src_dir_ + path_).data(), &mm_file_stat_);
    }
}

void http_response::add_state_line_(buffer& buff) {
    std::string status;
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void http_response::add_header_(buffer& buff) {
    buff.append("Connection: ");
    if(is_keep_alive_) {
        buff.append("Keep-Alive\r\n");
        buff.append("Keep-Alive: timeout=10000\r\n");
    } else{
        buff.append("close\r\n");
    }
    buff.append("Content-type: " + get_file_type_() + "\r\n");
}

void http_response::add_content_(buffer& buff) {
    int src_fd = open((src_dir_ + path_).data(), O_RDONLY);
    if(src_fd < 0) { 
        error_content(buff, "File NotFound!");
        return; 
    }

    /* 将文件映射到内存提高文件的访问速度 
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    LOG_DEBUG("file path %s", (src_dir_ + path_).data());
    int* mmRet = (int*)mmap(0, mm_file_stat_.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    if(*mmRet == -1) {
        error_content(buff, "File NotFound!");
        return; 
    }
    mm_file_ = (char*)mmRet;
    close(src_fd);
    buff.append("Content-length: " + std::to_string(mm_file_stat_.st_size) + "\r\n\r\n");
}

void http_response::unmap_file() {
    if(mm_file_) {
        munmap(mm_file_, mm_file_stat_.st_size);
        mm_file_ = nullptr;
    }
}

std::string http_response::get_file_type_() {
    /* 判断文件类型 */
    std::string::size_type idx = path_.find_last_of('.');
    if(idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = path_.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void http_response::error_content(buffer& buff, std::string message) 
{
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}
