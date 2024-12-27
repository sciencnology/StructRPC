#include "functions.hpp"
#include <utility>
#include <iostream>
#include <thread>
#include <vector>


int main()
{
    TCPServer server(/* thread_num */ 2, /* listen_port */ 8080);
    server.RegisterServerFunctions<&ExampleRPCClass::add, 
        &ExampleRPCClass::coro, 
        &free_add, 
        &ExampleRPCClass::generic_add<int>, 
        &ExampleRPCClass::generic_add_various_params<int, int, double>,
        &ExampleRPCClass::generic_add_various_params<int, int>>();
    server.Start();
    return 0;
}