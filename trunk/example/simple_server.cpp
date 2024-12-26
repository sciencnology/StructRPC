#include "functions.hpp"
#include <utility>
#include <iostream>
#include <thread>
#include <vector>


int main()
{
    try
    {
        TCPServer server(2, 8080);
        server.RegisterServerFunctions<&TCPTestServer::add, 
            &TCPTestServer::coro, 
            &free_add, 
            &TCPTestServer::static_sub, 
            &TCPTestServer::generic_add_various_params<int, int, double>,
            &TCPTestServer::generic_add_various_params<int, int>>();
        server.Start();
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}