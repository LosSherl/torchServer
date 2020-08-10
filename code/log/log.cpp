#include "log.h"

Log::Log() {
    line_count_ = 0;
    is_async_ = false;
    write_thread_ = nullptr;
    deque_ = nullptr;
    today_ = 0;
    fp_ = nullptr; 
    is_open_ = false;
}

Log::~Log() {
    if(write_thread_ && write_thread_->joinable()) {
        while(!deque_->empty()) {
            deque_->flush();
        }
        deque_->close();
        write_thread_->join();
    }
    if(fp_) {
        std::lock_guard<std::mutex> locker(mtx_);
        flush();
        fclose(fp_);
    }
}

int Log::get_level() {
    std::lock_guard<std::mutex> locker(mtx_);
    return level_;
}

void Log::set_level(int level) {
    std::lock_guard<std::mutex> locker(mtx_);
    level_ = level;
}

void Log::init(int level, const char* path,
                const char* suffix, int max_queue_cap) {
    if(is_open_)
        return;
    is_open_ = true;
    level_ = level;
    if(max_queue_cap > 0) {
        is_async_ = true;
        if(deque_ == nullptr) {
            std::unique_ptr<block_deque<std::string>> new_deque(new block_deque<std::string>());
            deque_ = std::move(new_deque);

            std::unique_ptr<std::thread> new_thread(new std::thread(flush_log_thread));
            write_thread_ = std::move(new_thread);
        }
    }
    else {
        is_async_ = false;
    }

    line_count_ = 0;

    time_t timer = time(nullptr);
    struct tm* sys_time = localtime(&timer);
    struct tm t = *sys_time;

    path_ = path;
    suffix_ = suffix;
    
    char file_name[LOG_NAME_LEN] = {0};
    snprintf(file_name, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d_%02d_%02d%s", path_, 
                t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, suffix_);
    today_ = t.tm_mday;
    {
        std::lock_guard<std::mutex> locker(mtx_);
        buff_.retrieve_all();
        if(fp_) { 
            fflush(fp_);
            fclose(fp_); 
        }

        fp_ = fopen(file_name, "a");
        if(fp_ == nullptr) {
            mkdir(path_, 0777);
            fp_ = fopen(file_name, "a");
        } 
        assert(fp_ != nullptr);
    }
}

void Log::write(int level, const char* format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    /* 日志日期 日志行数 */
    if (today_ != t.tm_mday || (line_count_ && (line_count_  %  MAX_LINES == 0)))
    {
        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        // snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        snprintf(tail, 36, "%04d_%02d_%02d_%02d_%02d", t.tm_year + 1900, 
            t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min);

        if (today_ != t.tm_mday)
        {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            today_ = t.tm_mday;
            line_count_ = 0;
        }
        else {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (line_count_  / MAX_LINES), suffix_);
        }
        std::lock_guard<std::mutex> locker(mtx_);
        fflush(fp_);
        fclose(fp_);
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }

    {
        std::lock_guard<std::mutex> locker(mtx_);
        line_count_++;
        int n = snprintf(buff_.begin_write(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
                    
        buff_.has_written(n);
        append_log_level_(level);

        va_start(vaList, format);
        int m = vsnprintf(buff_.begin_write(), buff_.writable_bytes(), format, vaList);
        va_end(vaList);

        buff_.has_written(m);
        buff_.append("\n\0", 2);

        if(is_async_ && deque_ && !deque_->full()) {
            deque_->push_back(buff_.retrieve_all_to_str());
        } else {
            fputs(buff_.peek(), fp_);
        }
        buff_.retrieve_all();
    }
}

void Log::append_log_level_(int level) {
    switch(level) {
    case 0:
        buff_.append("[debug]: ", 9);
        break;
    case 1:
        buff_.append("[info] : ", 9);
        break;
    case 2:
        buff_.append("[warn] : ", 9);
        break;
    case 3:
        buff_.append("[error]: ", 9);
        break;
    default:
        buff_.append("[info] : ", 9);
        break;
    }
}

void Log::flush() {
    {
        std::lock_guard<std::mutex> locker(mtx_);
        fflush(fp_);
    }

    if(is_async_) {
        deque_->flush();
    }
}

void Log::async_write_() {
    std::string str;
    while(deque_->pop_front(str)) {
        std::lock_guard<std::mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}

Log* Log::instance() {
    static Log log;
    return &log;
}

void Log::flush_log_thread() {
    Log::instance()->async_write_();
}