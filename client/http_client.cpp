#include "http_client.h"


http_client::http_client(const std::string& host, int port) {
    host_ = host;
    port_ = port;
}

void http_client::send_post() {
    init_socket_();
    std::string request = std::move(get_request_("/result"));
    send_request_(request);
}

void http_client::init_socket_() {
	sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
	addr_.sin_family = AF_INET;
	addr_.sin_port = htons(port_);
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
	std::cout << "client 接收数据: " << std::endl << buffer_ << std::endl;
}

std::string http_client::get_request_(const std::string& path) {
	// POST请求方式
	std::string request;
	std::fstream f("base64_url_img.txt");
	std::string base64_img;
	f >> base64_img;
	// std::cout << base64_img.length() << std::endl;
	request += "POST " + path;
	request += " HTTP/1.0\r\n";
	request += "Host: " + host_ + "\r\n";
	request += "User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n";
	request += "Content-Type: application/x-www-form-urlencoded\r\n";
	request += "Connection: close\r\n\r\n";
	request += "img_str=";
    return request + base64_img;
}