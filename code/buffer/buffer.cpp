#include "buffer.h"

buffer::buffer(int init_buffer_size) : buffer_(init_buffer_size), read_pos_(0), write_pos_(0) {}

size_t buffer::readable_bytes() const {
    return write_pos_ - read_pos_;
}

size_t buffer::writable_bytes() const {
    return buffer_.size() - write_pos_;
}

size_t buffer::prependable_bytes() const {
    return read_pos_;
}

const char* buffer::peek() const {
    return begin_ptr_() + read_pos_;
}

void buffer::retrieve(size_t len) {
    assert(len <= readable_bytes());
    read_pos_ += len;
}

void buffer::retrieve_until(const char* end) {
    assert(peek() <= end);
    retrieve(end - peek());
}

void buffer::retrieve_all() {
    bzero(&buffer_[0], buffer_.size());
    read_pos_ = write_pos_ = 0;
}

std::string buffer::retrieve_all_to_str() {
    std::string str(peek(), readable_bytes());
    retrieve_all();
    return str;
}

const char* buffer::begin_write_const() const {
    return begin_ptr_() + write_pos_;
}

char* buffer::begin_write() {
    return begin_ptr_() + write_pos_;
}

void buffer::has_written(size_t len) {
    write_pos_ += len;
}

void buffer::append(const char* str, size_t len) {
    assert(str);
    ensure_writable(len);
    std::copy(str, str + len, begin_write());
    has_written(len);
}

void buffer::append(const std::string& str) {
    append(str.data(), str.length());
}

void buffer::append(const void* data, size_t len) {
    assert(data);
    append(static_cast<const char*>(data), len);
}

void buffer::append(const buffer& buff) {
    append(buff.peek(), buff.readable_bytes());
}

void buffer::ensure_writable(size_t len) {      //扩容保证大小
    if(writable_bytes() < len) {
        make_space_(len);
    }
    assert(writable_bytes() >= len);
}

ssize_t buffer::read_fd(int fd, int* save_errno) {
    char buff[65535] = {0};
    struct iovec iov[2];
    const size_t writable = writable_bytes();
    // 分散读 保证读完
    iov[0].iov_base = begin_ptr_() + write_pos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);
    
    const ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *save_errno = errno;
    }
    else if(static_cast<size_t>(len) <= writable) {
        write_pos_ += len;
    }
    else {
        write_pos_ = buffer_.size();
        append(buff, len - write_pos_);
    }
    return len;
}

ssize_t buffer::write_fd(int fd, int* save_errno) {
    size_t readable_size = readable_bytes();
    ssize_t len = write(fd, peek(), readable_size);
    if(len < 0) {
        *save_errno = errno;
        return len;
    }
    read_pos_ += len;
    return len;
}

char* buffer::begin_ptr_() {
    return &*buffer_.begin();
}

const char* buffer::begin_ptr_() const {
    return &*buffer_.begin();
}

void buffer::make_space_(size_t len) {
    if(writable_bytes() + prependable_bytes() < len) {  //  已读大小+可写大小不可以容纳
        buffer_.resize(write_pos_ + len + 1);
    }
    else {              //  未读数据往前挪
        size_t readable_size = readable_bytes();
        std::copy(begin_ptr_() + read_pos_, begin_ptr_() + write_pos_, begin_ptr_());
        read_pos_ = 0;
        write_pos_ = read_pos_ + readable_size;
        assert(readable_size == readable_bytes());
    }
}



