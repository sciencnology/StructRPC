#include <utility>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
// #include <boost/asio/experimental/awaitable_operators.hpp>
#include "nlohmann/json.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include "moodycamel/concurrentqueue.h" // 使用 moodycamel::ConcurrentQueue

#include "http_server.hpp"
#include "tcp_server.hpp"
#include "TCPTestServer.hpp"

int main()
{
    try
    {
        int port = 0;
        std::cin >> port;

        TCPServer server(2, port);
        server.RegisterServerFunctions<&TCPTestServer::add, &TCPTestServer::echo, &TCPTestServer::coro>();
        server.Start();
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}