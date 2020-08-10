#include "http_client.h"


http_client::http_client(const std::string& host, int node_cnt, const std::vector<int>& ports) {
    assert(node_cnt == ports.size() && node_cnt > 0);
    node_cnt_ = node_cnt;
	req_cnt_ = std::vector<int>(node_cnt_);
    host_ = host;
    server_ports_ = ports;
	uniform_ = std::uniform_int_distribution<int>(0, std::numeric_limits<int>::max());
    std::unique_ptr<c_hash> new_hasher(new c_hash(node_cnt, virtual_nodes_));
    c_hasher_ = std::move(new_hasher);
    c_hasher_->init();
}

void http_client::random_request() {
    int node_idx = uniform_(rand_engine_) % node_cnt_;
	send_(node_idx);
}

void http_client::poll_request(int index) {
    int node_idx = index % node_cnt_;
    send_(node_idx);
}

void http_client::c_hash_request() {
    int rand_val = uniform_(rand_engine_);
    std::string key = std::to_string(rand_val);
    int node_idx = c_hasher_->get_server_index(key);
    send_(node_idx);
}

void http_client::print_req_history() {
	for(int i = 0; i < node_cnt_; i++) {
		std::cout << "node" << i << ": " << req_cnt_[i] << std::endl;
	}
}

void http_client::reset_history() {
	for(int i = 0; i < node_cnt_; i++) {
		req_cnt_[i] = 0;
	}
}

void http_client::send_(int node_idx) {
	req_cnt_[node_idx] += 1;
    init_socket_(server_ports_[node_idx]);
    std::string request = std::move(get_request_("/"));
    send_request_(request);
}

void http_client::init_socket_(int port) {
	sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
	addr_.sin_family = AF_INET;
	addr_.sin_port = htons(port);
	inet_pton(AF_INET, host_.c_str(), &addr_.sin_addr);   //ip转网络字节序

	if(connect(sock_fd_, (struct sockaddr *)&addr_, sizeof(sockaddr_in)) < 0) {
		std::cout << "connect error : " << std::endl;
		return;
	}
	
}

void http_client::send_request_(const std::string& request) {
    send(sock_fd_, request.c_str(), request.size(), 0);

	// std::cout << "client 发送数据: " << request.c_str() << std::endl;
	memset(buffer_, 0, sizeof(buffer_));
	//  循环接收
	int offset = 0;
	int rc;
	while(rc = recv(sock_fd_, buffer_ + offset, 1024, 0)) {
		offset += rc;
	}

	close(sock_fd_);

	buffer_[offset] = 0;
	// std::cout << "client 接收数据: " << std::endl << buffer_ << std::endl;
}

std::string http_client::get_request_(const std::string& path) {
	// GET请求方式
	std::string request;
	request += "GET " + path;
	request += " HTTP/1.0\r\n";
	request += "Host: " + host_ + "\r\n";
	request += "User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n";
	request += "Connection:close\r\n\r\n";
    return request;
}