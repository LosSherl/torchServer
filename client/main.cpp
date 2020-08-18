#include <string>
#include <vector>
#include <memory>

#include "http_client.h"


int main(int argc, char* argv[]) {
    int port = 1316;
    const std::string host = "223.3.81.121";
    std::unique_ptr<http_client> client(new http_client(host, port));
    client->send_post();
}