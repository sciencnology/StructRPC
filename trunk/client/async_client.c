#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <string>

// g++ async_client.c -lboost_system -lboost_thread -lpthread -I/usr/local/include -I/usr/include -std=c++20
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

asio::awaitable<void> perform_request(asio::io_context &io_context, const std::string &host, const std::string &port, const std::string &target)
{
    try
    {
        tcp::resolver resolver(io_context);
        beast::tcp_stream stream(io_context);

        // Resolve the host and port
        auto const results = co_await resolver.async_resolve(host, port, asio::use_awaitable);

        // Connect to the resolved endpoint
        co_await stream.async_connect(results, asio::use_awaitable);

        // Create the HTTP GET request
        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        co_await http::async_write(stream, req, asio::use_awaitable);

        // Buffer for reading the response
        beast::flat_buffer buffer;
        // Container for the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        co_await http::async_read(stream, buffer, res, asio::use_awaitable);

        // Output the response
        std::cout << res << std::endl;

        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        if (ec && ec != beast::errc::not_connected)
        {
            throw beast::system_error{ec};
        }
    }
    catch (std::exception const &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main()
{
    try
    {
        asio::io_context io_context;

        // Modify the host and port as needed
        std::string host = "localhost";
        std::string port = "8080"; // Change to the desired port number
        std::string target = "/";  // Change to the desired target path

        asio::co_spawn(io_context, perform_request(io_context, host, port, target), asio::detached);

        io_context.run();
    }
    catch (std::exception const &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
