#ifndef C_HASH_H
#define C_HASH_H

#include <map>
#include <string.h>
#include <sstream>

class c_hash
{
public:
    c_hash(int node_num, int virtual_node_num);
    ~c_hash();

    void init();
    size_t get_server_index(const std::string& key);

    void del_node(const int index);
    void add_node(const int index);

private:
    std::map<size_t, size_t> nodes_to_server_; // 虚拟节点, key是哈希值，value是服务节点的index
    int node_num_;  //真实服务节点个数
    int virtual_node_num_;  //每个服务节点关联的虚拟节点个数
    std::hash<std::string> str_hasher_;
};


#endif