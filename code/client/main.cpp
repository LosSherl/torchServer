#include <string>
#include <vector>
#include <memory>

#include "http_client.h"


int main(int argc, char* argv[]) {
    assert(argc >= 2);
    const int test_cnt = 1000;
    std::vector<int> ports = {1316, 1317, 1318};
    int node_cnt = atoi(argv[1]);
    const std::string host = "223.3.81.121";
    std::unique_ptr<http_client> client(new http_client(host, node_cnt, ports));

    /* 一致性哈希 */
    for(int i = 0; i < 1000; i++) {
        client->c_hash_request();;
    }
    std::cout << "Consistent Hash:" << std::endl;
    client->print_req_history();
    client->reset_history();

    /* 随机 */
    for(int i = 0; i < 1000; i++) {
        client->random_request();;
    }
    std::cout << "Random Query:" << std::endl;
    client->print_req_history();
    client->reset_history();

    /* 轮询 */
    for(int i = 0; i < 1000; i++) {
        client->poll_request(i);;
    }
    std::cout << "Polling Query:" << std::endl;
    client->print_req_history();
}