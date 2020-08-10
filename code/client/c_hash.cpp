#include "c_hash.h"

c_hash::c_hash(int node_num, int virtual_node_num)
{
    node_num_ = node_num;
    virtual_node_num_ = virtual_node_num;
}

c_hash::~c_hash()
{
    nodes_to_server_.clear();
}


void c_hash::init()
{   
    std::hash<std::string> str_hasher;
    for(int i = 0; i < node_num_; i++)
    {
        for(int j = 0; j < virtual_node_num_; j++)
        {
            std::string node_key = "SHARD-" + std::to_string(i) + "-NODE-" + std::to_string(j);
            size_t partition = str_hasher_(node_key);
            nodes_to_server_.emplace(partition, i);
        }
    }
}


size_t c_hash::get_server_index(const std::string& key)
{
    size_t partition = str_hasher_(key);
    std::map<size_t, size_t>::iterator it = nodes_to_server_.lower_bound(partition);  //沿环的顺时针找到一个大于等于key的虚拟节点

    if(it == nodes_to_server_.end())    //未找到
    {
        return nodes_to_server_.begin()->second;    // 返回第一个
    }
    return it->second;
}


void c_hash::del_node(const int index)
{
    for(int j = 0; j < virtual_node_num_; ++j)
    {
        std::string node_key = "SHARD-" + std::to_string(index) + "-NODE-" + std::to_string(j);
        size_t partition = str_hasher_(node_key);
        std::map<size_t, size_t>::iterator it = nodes_to_server_.find(partition);
        if(it != nodes_to_server_.end())
        {
            nodes_to_server_.erase(it);
        }
    }
}

void c_hash::add_node(const int index)
{
    for(int j = 0; j < virtual_node_num_; j++)
    {
        std::string node_key = "SHARD-" + std::to_string(index) + "-NODE-" + std::to_string(j);
        size_t partition = str_hasher_(node_key);
        nodes_to_server_.emplace(partition, index);
    }
}