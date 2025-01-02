#include "functions.hpp"
#include <utility>
#include <iostream>
#include <thread>
#include <vector>


int main()
{
    uint32_t thread_num = std::stoi(argv[1]);
    uint32_t port = std::stoi(argv[2]);

    TCPServer server(/* thread_num */ thread_num, /* listen_port */ port);
    server.RegisterServerFunctions<&rpc_benchmark::echo>();
    server.Start();
    return 0;
}