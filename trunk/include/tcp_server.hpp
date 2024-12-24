#pragma once
#include <utility>
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
#include "trait_helper/func_name.hpp"
#include "trait_helper/func_traits.hpp"
#include "tuple_interface.h"

using namespace boost::asio;
using tcp = ip::tcp;

class TCPServer
{
private:
    io_context io_ctx;
    uint32_t thread_num = 0;
    uint32_t port = 0;
    boost::asio::thread_pool thread_pool;
    TCPProcessCoroutineMap process_coroutine_map;
    TCPProcessFuncMap process_function_map;

private:
    awaitable<TCPResponse> process_request(TCPRequest tcp_request)
    {
        TCPResponse tcp_response;
        auto coroutine_path_iter = process_coroutine_map.find(std::move(tcp_request.path));
        if (coroutine_path_iter != process_coroutine_map.end())
        {
            auto& func = coroutine_path_iter->second;
            tcp_response.data = co_await func(tcp_request.data);
        } else {
            auto func_path_iter = process_function_map.find(std::move(tcp_request.path));
            if (func_path_iter != process_function_map.end())
            {
                auto& func = func_path_iter->second;
                tcp_response.data = func(tcp_request.data);
            } else {
                tcp_response.retcode = 1;
            }
        }

        co_return tcp_response;
    };
    // 用于处理单个 TCP 客户端连接的协程
    awaitable<void> handle_client(tcp::socket socket)
    {
        try
        {
            for (;;)
            {
                size_t total_size;

                co_await async_read(socket, asio::mutable_buffer(&total_size, sizeof(size_t)), use_awaitable);

                std::string request_str(total_size, '\0');
                std::memcpy(request_str.data(), &total_size, sizeof(size_t));

                co_await async_read(socket, asio::mutable_buffer(request_str.data() + sizeof(size_t), total_size - sizeof(size_t)), use_awaitable);
                
                std::cout << "Received size: " << total_size << std::endl;
                TCPRequest tcp_request;
                structbuf::deserializer::ParseFromSV(tcp_request, request_str);

                TCPResponse tcp_response = co_await process_request(std::move(tcp_request));
                std::string response_str = structbuf::serializer::SaveToString(tcp_response);
                
                co_await async_write(socket, asio::buffer(response_str, response_str.size()), use_awaitable);
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
    TCPServer(uint32_t thread_num, uint32_t port = 8080) : thread_num(thread_num), port(port), thread_pool(thread_num)
    {
    }

    void Start()
    {
        tcp::endpoint endpoint(tcp::v4(), port);
        tcp::acceptor acceptor(io_ctx, endpoint);
        co_spawn(io_ctx, acceptor_coroutine(std::move(acceptor)), detached);
        signal_set signals(io_ctx, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto)
                           { io_ctx.stop(); });
        // 使用线程池运行 IO 上下文
        for (int i = 0; i < thread_num; ++i)
        {
            boost::asio::post(thread_pool, [&]
                              { io_ctx.run(); });
        }

        thread_pool.join();
        std::cout << "Server stopped." << std::endl;
    }

    template <auto Func>
    void RegisterSingleFunction(TCPProcessCoroutineMap& process_coroutine_map, TCPProcessFuncMap& process_func_map)
    {
        if constexpr (trait_helper::function_helper::is_asio_coroutine<decltype(Func)>) {
            // process_coroutine_map[trait_helper::func_name<Func>()] = [this](std::string_view input) -> asio::awaitable<std::string> { co_return co_await TupleInterface::CommonCoroutineTemplate<Func>(input); };
            process_coroutine_map[trait_helper::func_name<Func>()] = TupleInterface::CommonCoroutineTemplate<Func>;
        } else {
            // process_func_map[trait_helper::func_name<Func>()] = [this](std::string_view input) -> std::string { return TupleInterface::CommonFuncTemplate<Func>(input); };
            process_func_map[trait_helper::func_name<Func>()] = TupleInterface::CommonFuncTemplate<Func>;
        }
    }

    template <auto... Funcs>
    constexpr void RegisterServerFunctions()
    {
        (RegisterSingleFunction<Funcs>(process_coroutine_map, process_function_map), ...);
    }
};
