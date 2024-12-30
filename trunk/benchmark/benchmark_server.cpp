#include "functions.hpp"
#include <utility>
#include <iostream>
#include <thread>
#include <vector>


int main()
{
    uint32_t thread_num = 2;
    TCPServer server(/* thread_num */ thread_num, /* listen_port */ 8080);
    server.RegisterServerFunctions<&rpc_benchmark::echo>();
    server.Start();
    return 0;
}