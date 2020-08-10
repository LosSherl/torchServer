#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

class http_response {
public:
    http_response();
    ~http_response();

    void init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    void make_response(buffer& buff);
    void unmap_file();
    char* file();
    size_t file_len() const;
    void error_content(buffer& buff, std::string message);
    int code() const { return code_; }

private:
    void add_state_line_(buffer &buff);
    void add_header_(buffer &buff);
    void add_content_(buffer &buff);

    void error_html_();
    std::string get_file_type_();

    int code_;
    bool is_keep_alive_;

    std::string path_;
    std::string src_dir_;
    
    char* mm_file_; 
    struct stat mm_file_stat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};


#endif 