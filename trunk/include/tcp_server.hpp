#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <thread>

#include "data_struct_define.hpp"
#include "so_wrapper.h"

using namespace boost::asio;
using tcp = ip::tcp;

class TCPSerer
{
private:
    io_context io_ctx;
    boost::asio::thread_pool thread_pool;
    TCPProcessCoroutineMap process_coroutine_map;

private:
    awaitable<TCPResponse> process_request(TCPRequest tcp_request)
    {
        TCPResponse tcp_response;
        auto class_iter = process_coroutine_map.find(std::move(tcp_request.class_name));
        if (class_iter == process_coroutine_map.end())
        {
            tcp_response.retcode = 1;
            co_return tcp_response;
        }

        auto &method_map = class_iter->second;
        auto method_iter = method_map.find(std::move(tcp_request.method));
        if (method_iter == method_map.end())
        {
            tcp_response.retcode = 2;
            co_return tcp_response;
        }

        auto &func_ = method_iter->second;
        tcp_response.data = co_await func_(tcp_request.data);

        co_return tcp_response;
    };
    // 用于处理单个 TCP 客户端连接的协程
    awaitable<void> handle_client(tcp::socket socket)
    {
        try
        {
            char data[1024];
            for (;;)
            {
                size_t total_size;

                co_await async_read(socket, buffer::mutable_buffer(&total_size, sizeof(size_t)), use_awaitable);

                std::string request_str(total_size);
                std::memcpy(request_str, &total_size, sizeof(size_t));

                co_await async_read(socket, buffer::mutable_buffer(request_str.data() + sizeof(size_t), total_size - sizeof(size_t)), use_awaitable);
                TCPRequest tcp_request;
                structbuf::deserializer::ParseFromSV(tcp_request, request_str);

                TCPResponse tcp_response = co_await process_request(std::move(tcp_request));

                // 将接收到的数据打印并回显
                std::cout
                    << "Received size: " << total_size << std::endl;
                co_await async_write(socket, buffer(data, n), use_awaitable);
            }
        }
        catch (std::exception &e)
        {
            std::cerr << "Connection error: " << e.what() << std::endl;
        }
    }

    // 用于接受客户端连接的协程
    awaitable<void> acceptor_coroutine(tcp::acceptor acceptor)
    {
        try
        {
            for (;;)
            {
                // 等待接受新的连接
                tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
                std::cout << "New client connected!" << std::endl;

                // 启动一个协程处理客户端
                co_spawn(io_ctx, handle_client(std::move(socket)), detached);
            }
        }
        catch (std::exception &e)
        {
            std::cerr << "Acceptor error: " << e.what() << std::endl;
        }
    }

public:
    TCPServer(uint32_t thread_num, uint32_t port = 8080) : thread_pool(thread_num)
    {
    }

    void Start()
    {
        tcp::endpoint endpoint(tcp::v4(), 8080);
        tcp::acceptor acceptor(io_ctx, endpoint);
        co_spawn(io_ctx, acceptor_coroutine(std::move(acceptor)), detached);
        signal_set signals(io_ctx, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto)
                           { io_ctx.stop(); });
        // 使用线程池运行 IO 上下文
        for (int i = 0; i < thread_count; ++i)
        {
            boost::asio::post(thread_pool, [&]
                              { io_ctx.run(); });
        }

        thread_pool.join();
        std::cout << "Server stopped." << std::endl;
    }
};

int main()
{
    try
    {
        // 初始化 IO 上下文和线程池
        const int thread_count = std::thread::hardware_concurrency();
        io_context::executor_type executor = io_ctx.get_executor();
        boost::asio::thread_pool thread_pool(thread_count);

        // 绑定 TCP 服务器端口
        tcp::endpoint endpoint(tcp::v4(), 8080);
        tcp::acceptor acceptor(io_ctx, endpoint);

        // 启动接受连接的协程
        co_spawn(io_ctx, acceptor_coroutine(std::move(acceptor)), detached);

        // 捕获 SIGINT 和 SIGTERM 信号，优雅地停止服务器
        signal_set signals(io_ctx, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto)
                           { io_ctx.stop(); });

        // 使用线程池运行 IO 上下文
        for (int i = 0; i < thread_count; ++i)
        {
            boost::asio::post(thread_pool, [&]
                              { io_ctx.run(); });
        }

        thread_pool.join();
        std::cout << "Server stopped." << std::endl;
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
