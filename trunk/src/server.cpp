#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include "nlohmann/json.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include "moodycamel/concurrentqueue.h" // 使用 moodycamel::ConcurrentQueue

#include "http_server.hpp"

int main()
{
    try
    {
        int port = 0;
        std::cin >> port;
        HttpServer server(2, port);
        server.start();
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}