#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP
#include <utility>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
// #include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/algorithm/string.hpp>
#include "nlohmann/json.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include "util.hpp"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;
using json = nlohmann::json;
// using namespace asio::experimental::awaitable_operators;

struct Request
{
    tcp::socket socket;
    http::request<http::string_body> req;

    Request(tcp::socket &&socket, http::request<http::string_body> &&req)
        : socket(std::move(socket)), req(std::move(req)) {}
};

class HttpServer
{
public:
    HttpServer(int thread_cnt, int port = 8080);

    void start();

    void stop();

    asio::awaitable<void> do_session(tcp::socket socket);

    asio::awaitable<void> do_listen(asio::io_context &ioc, tcp::endpoint endpoint);

private:
    asio::awaitable<void> process_single_request(std::shared_ptr<Request> request);

    template <http::status RSP_CODE>
    void do_response(boost::asio::any_io_executor executor, std::shared_ptr<Request> request, std::string_view body, unsigned int version);

private:
    int io_thread_cnt_ = 1;     // I/O 线程数
    int worker_thread_cnt_ = 1; // 业务逻辑线程数
    int thread_cnt_ = 2;        // 线程数
    int port_ = 8080;

    std::vector<std::shared_ptr<asio::io_context>> io_contexts_;
    std::vector<asio::executor_work_guard<asio::io_context::executor_type>> work_guards_;
    std::atomic<bool> running_{true};

    std::map<std::string, std::map<std::string, ProcessFunc>> proccess_func_map_;
    ProcessCoroutineMap proccess_coroutine_map_;
};

#endif // HTTP_SERVER_HPP
