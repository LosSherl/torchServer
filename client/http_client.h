#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <memory>
#include <vector>
#include <fstream>
#include <cassert>


class http_client {
public:
	http_client(const std::string& host, int port);
	~http_client() = default;
	void send_post();
	
private: 
	std::string get_request_(const std::string& path);
	void send_request_(const std::string& request);
	void init_socket_();

private:
	
	std::string host_;
	struct sockaddr_in addr_;
	int sock_fd_;
	int port_;
	char buffer_[65535]; 
};

#endif