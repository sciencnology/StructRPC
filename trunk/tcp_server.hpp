#pragma once
#include <utility>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>  // boost requirement: 1.80.0
#include <iostream>
#include <string>
#include <string_view>
#include <memory>
#include <thread>
#include <map>

#include "common_define.hpp"
#include "utils/trait_helper/trait_helper.hpp"
#include "utils/logger.hpp"

namespace struct_rpc
{
/**
 * @class TCPServer: 基于C++20协程封装boost::asio的多线程异步TCP服务器 
*/
class TCPServer
{
    using tcp = ip::tcp;
    using TCPProcessCoroutine = std::function<asio::awaitable<std::string>(std::string_view)>;
    using TCPProcessFunc = std::function<std::string(std::string_view)>;
    using TCPProcessCoroutineMap = std::map<std::string_view, TCPProcessCoroutine>;
    using TCPProcessFuncMap = std::map<std::string_view, TCPProcessFunc>;
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
        for (int i = 0; i < thread_num; ++i)
        {
            boost::asio::post(thread_pool, [&]
                              { io_ctx.run(); });
        }
        thread_pool.join();
        LOG("server stopped");
    }

    /**
     * @brief: 批量向server注册RPC处理函数
     * @param Funcs: 可变数量非类型模板参数，期望传入对应的函数指针
    */
    template <auto... Funcs>
    constexpr void RegisterServerFunctions()
    {
        (RegisterSingleFunction<Funcs>(process_coroutine_map, process_function_map), ...);
    }

private:
    /**
     * @brief: 处理单次RPC请求并返回对应结果
    */
    awaitable<common_define::TCPResponse> process_request(common_define::TCPRequest tcp_request)
    {
        common_define::TCPResponse tcp_response {0, ""};
        auto coroutine_path_iter = process_coroutine_map.find(tcp_request.path);
        // C++20标准无法统一协程和普通函数的调用，故放在两个map里分别查询和调用
        if (coroutine_path_iter != process_coroutine_map.end())
        {
            auto& func = coroutine_path_iter->second;
            tcp_response.data = co_await func(tcp_request.params);
        } else {
            auto func_path_iter = process_function_map.find(tcp_request.path);
            if (func_path_iter != process_function_map.end()) {
                auto& func = func_path_iter->second;
                tcp_response.data = func(tcp_request.params);
            } else {
                tcp_response.retcode = static_cast<int32_t>(common_define::RetCode::RET_NOT_FOUND);
            }
        }
        LOG("succ to process req path={}", tcp_request.path);
        co_return tcp_response;
    };

    /**
     * @brief: 进行一个附带超时时间的异步操作。如果超时则关闭socket并返回false
    */
    awaitable<std::pair<bool, std::string>> async_operation_with_timeout(auto&& async_op, tcp::socket& socket, uint32_t timeout_seconds) {
        using namespace boost::asio::experimental::awaitable_operators;
        auto executor = co_await this_coro::executor;
        steady_timer timer(executor);
        timer.expires_after(std::chrono::seconds(timeout_seconds));
        // awaitable重载了operator ||, 下面任何一个异步操作完成都会返回，并取消另一个。
        auto result = co_await (std::forward<decltype(async_op)>(async_op) || timer.async_wait(use_awaitable));
        if (result.index() == 0) {
            co_return std::pair{true, ""};
        } else {
            socket.cancel();
            co_return std::pair{false, "timed out"};
        }
    }

    /**
     * @brief: 用于处理单个 TCP 客户端连接的协程。客户端达到超时时间且无请求会关闭，实现超时自动退出的连接池
    */
    awaitable<void> handle_client(tcp::socket socket, uint32_t timeout_seconds = 5)
    {
        auto remote_endpoint = socket.remote_endpoint();
        std::string remote_info = std::format("host={}, port={}", remote_endpoint.address().to_string(),  std::to_string(remote_endpoint.port()));
        LOG("connected with client {}", remote_info);
        for (;;)
        {
            // step 1. 读取TCP请求序列化的头部（包含整个请求包长度信息）
            size_t total_size;
            if (auto [succ, msg] = co_await async_operation_with_timeout(
                async_read(socket, asio::mutable_buffer(&total_size, sizeof(size_t)), use_awaitable),
                socket,
                timeout_seconds
            ); !succ) {
                LOG("client {} async read msg head failed with {}, destroy this corotine", remote_info, msg);
                co_return;
            }

            // step 2. 读取整个TCP请求结构体
            std::string request_str(total_size + sizeof(size_t), '\0');
            std::memcpy(request_str.data(), &total_size, sizeof(size_t));
            if (auto [succ, msg] = co_await async_operation_with_timeout(
                async_read(socket, asio::mutable_buffer(request_str.data() + sizeof(size_t), total_size), use_awaitable),
                socket,
                timeout_seconds
            ); !succ) {
                LOG("client {} async read msg body failed with {}, destroy this corotine",remote_info, msg);
                co_return;
            }
            
            // step 3. 根据请求中编码的path调用对应的RPC函数
            common_define::TCPResponse tcp_response;
            try
            {
                common_define::TCPRequest tcp_request;
                structbuf::deserializer::ParseFromSV(tcp_request, request_str);
                tcp_response = co_await process_request(std::move(tcp_request));
            }
            catch (std::exception& e)
            {
                LOG("server process exception {}", e.what());
                tcp_response.retcode = static_cast<int32_t>(common_define::RetCode::RET_SERVER_EXCEPTION);
            }
            
            // step 4. 写回调用结果
            std::string response_str = structbuf::serializer::SaveToString(tcp_response);
            if (auto [succ, msg] = co_await async_operation_with_timeout(
                async_write(socket, asio::buffer(response_str, response_str.size()), use_awaitable),
                socket,
                timeout_seconds
            ); !succ) {
                LOG("client {} async write response failed with {}, destroy this corotine",remote_info, msg);
                co_return;
            }
        }
    }

    /**
     * @brief: acceptor coroutine
    */
    awaitable<void> acceptor_coroutine(tcp::acceptor acceptor)
    {
        for (;;) {
            try
            {
                tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
                co_spawn(io_ctx, handle_client(std::move(socket)), [](std::exception_ptr e) {
                    try {
                        if (e) { std::rethrow_exception(e); }        
                    }
                    catch (std::exception &e) {
                        LOG("handle_client exception {}, close connection", e.what());
                    }
                });
            } catch (std::exception &e) {
                LOG("accept with exception {}, skip", e.what());
            }
        }
    }

    /**
     * @brief: 注册单个RPC函数
     * @param Func: RPC函数指针
    */
    template <auto Func>
    void RegisterSingleFunction(TCPProcessCoroutineMap& process_coroutine_map, TCPProcessFuncMap& process_func_map)
    {
        constexpr std::string_view path = trait_helper::struct_rpc_func_path<Func>();
        if constexpr (trait_helper::is_asio_coroutine<decltype(Func)>) {
            process_coroutine_map[path] = common_define::CommonCoroutineTemplate<Func>;
        } else {
            process_func_map[path] = common_define::CommonFuncTemplate<Func>;
        }
        LOG("registered func path {}", path);
    }

private:
    io_context io_ctx;  // asio io_context
    uint32_t thread_num = 0;    // server框架中不区分IO和工作线程，所有IO和其他阻塞全部采用协程异步进行
    uint32_t port = 0;
    boost::asio::thread_pool thread_pool;
    TCPProcessCoroutineMap process_coroutine_map;
    TCPProcessFuncMap process_function_map;
};
}
