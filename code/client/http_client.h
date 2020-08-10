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
#include <cassert>
#include <random>
#include <limits>

#include "c_hash.h"

class http_client {
public:
	http_client(const std::string& host, int node, const std::vector<int>& port);
	~http_client() = default;
	
	void random_request();

	void poll_request(int index);

	void c_hash_request();

	void print_req_history();

	void reset_history();

private: 
	std::string get_request_(const std::string& path);
	void send_request_(const std::string& request);
	void send_(int node_idx);

private:
	void init_socket_(int port);
	std::string host_;
	struct sockaddr_in addr_;
	int sock_fd_;
	int node_cnt_;
	std::vector<int> req_cnt_;
	std::vector<int> server_ports_;
	std::unique_ptr<c_hash> c_hasher_;
	std::default_random_engine rand_engine_;
	std::uniform_int_distribution<int> uniform_;
	char buffer_[1024 * 3]; 
	const int virtual_nodes_ = 100;
};

#endif