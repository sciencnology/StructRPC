#pragma once

#include <utility>
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include "common_define.hpp"
#include "utils/trait_helper/trait_helper.hpp"

namespace struct_rpc{
using namespace std::chrono_literals;
using tcp = asio::ip::tcp;


class TCPConnectionBase
{
public:
    std::string host;
    std::string port;
    TCPConnectionBase(std::string host, std::string port): host(host), port(port) {}
    virtual ~TCPConnectionBase() {};
    virtual std::string make_sync_tcp_request(std::string tcp_request) { throw std::runtime_error("not implemented"); }
    virtual asio::awaitable<std::string> make_async_tcp_request(std::string tcp_request) { throw std::runtime_error("not implemented"); }
    virtual void connect() {};
    virtual asio::awaitable<void> async_connect() { co_return; };

    /**
     * @brief: 进行一次同步RPC调用
    */
    template <auto Func, typename... Args>
    auto sync_struct_rpc_request(Args&&... args) {
        // step 1. 提取出RPC函数的参数类型列表，并完美转发输入的参数列表构造对应类型的tuple
        using param_tuple_type = typename trait_helper::function_traits<decltype(Func)>::arguments_tuple;
        param_tuple_type param_tuple = std::make_tuple(std::forward<Args>(args)...);
        // step 2. 提取出RCP调用路径，和序列化后的参数tuple构造TCP请求对象
        common_define::TCPRequest tcp_request {std::string(trait_helper::struct_rpc_func_path<Func>()), structbuf::serializer::SaveToString(param_tuple)};

        // step 3. 执行TCP请求，得到TCP响应对象
        std::string response_str;
        try
        {
            response_str = make_sync_tcp_request(structbuf::serializer::SaveToString(tcp_request));
        }
        catch(const std::exception& e)
        {
            // 请求失败可能是由于超时server关闭连接导致的，再次连接后重试一次
            connect();
            response_str = make_sync_tcp_request(structbuf::serializer::SaveToString(tcp_request));
        }
        
        common_define::TCPResponse tcp_response;
        structbuf::deserializer::ParseFromSV(tcp_response, response_str);
        if (tcp_response.retcode != 0) {
            throw std::runtime_error("errcode" + std::to_string(tcp_response.retcode) + tcp_request.path);
        }

        // step 4. 从TCP响应对象中提取出RCP的返回结果
        using ReturnType = typename trait_helper::rpc_return_type_getter<decltype(Func)>::type;
        ReturnType function_return_obj;
        structbuf::deserializer::ParseFromSV(function_return_obj, tcp_response.data);
        return function_return_obj;
    }

    /**
     * @brief: 进行一次异步RPC调用
    */
    template <auto Func, typename... Args>
    auto async_struct_rpc_request(Args&&... args) 
        -> awaitable<typename trait_helper::rpc_return_type_getter<decltype(Func)>::type>
    {
        using param_tuple_type = typename trait_helper::function_traits<decltype(Func)>::arguments_tuple;
        param_tuple_type param_tuple = std::make_tuple(std::forward<Args>(args)...);
        common_define::TCPRequest tcp_request {std::string(trait_helper::struct_rpc_func_path<Func>()), structbuf::serializer::SaveToString(param_tuple)};
        std::string response_str;
        bool need_retry = false;
        try
        {
            response_str = co_await make_async_tcp_request(structbuf::serializer::SaveToString(tcp_request));
        }
        catch(const std::exception& e)
        {
            // 请求失败可能是由于超时server关闭连接导致的，再次连接后重试一次
            need_retry = true;
        }

        if (need_retry) {
            co_await async_connect();
            response_str = co_await make_async_tcp_request(structbuf::serializer::SaveToString(tcp_request));
        }

        common_define::TCPResponse tcp_response;
        structbuf::deserializer::ParseFromSV(tcp_response, response_str);
        if (tcp_response.retcode != 0) {
            throw std::runtime_error("errcode" + std::to_string(tcp_response.retcode) + tcp_request.path);
        }
        using ReturnType = typename trait_helper::rpc_return_type_getter<decltype(Func)>::type;
        ReturnType function_return_obj;
        structbuf::deserializer::ParseFromSV(function_return_obj, tcp_response.data);

        co_return function_return_obj;
    }
};

class SyncTCPConnection : public TCPConnectionBase
{
private:
    boost::asio::io_context io_context;
    tcp::socket s;
public:
    SyncTCPConnection(std::string host, std::string port): TCPConnectionBase(host, port), s(io_context) 
    {
        connect();
    }

    void connect() override
    {
        tcp::resolver resolver(io_context);
        boost::asio::connect(s, resolver.resolve(host, port));
    }

    std::string make_sync_tcp_request(std::string tcp_request) override
    {
        boost::asio::write(s, boost::asio::buffer(tcp_request, tcp_request.size()));
        size_t total_size;
        boost::asio::read(s, boost::asio::buffer(&total_size, sizeof(size_t)));
        std::string response_str(total_size + sizeof(size_t), '\0');
        std::memcpy(response_str.data(), &total_size, sizeof(size_t));
        boost::asio::read(s, boost::asio::buffer(response_str.data() + sizeof(size_t), total_size));
        return response_str;
    }

};

class AsyncTCPConnection : public TCPConnectionBase
{
private:
    boost::asio::io_context& io_context;
    tcp::socket s;
public:
    AsyncTCPConnection(std::string host, std::string port, boost::asio::io_context& ioc): TCPConnectionBase(host, port), io_context(ioc), s(io_context) 
    {
    }

    awaitable<void> async_connect()
    {
        tcp::resolver resolver(s.get_executor());
        co_await asio::async_connect(s, resolver.resolve(host, port), asio::use_awaitable);
    }

    awaitable<std::string> make_async_tcp_request(std::string tcp_request) override {
        co_await boost::asio::async_write(s, boost::asio::buffer(tcp_request, tcp_request.size()), asio::use_awaitable);
        size_t total_size;
        co_await boost::asio::async_read(s, boost::asio::mutable_buffer(&total_size, sizeof(size_t)), asio::use_awaitable);
        std::string response_str(total_size + sizeof(size_t), '\0');
        std::memcpy(response_str.data(), &total_size, sizeof(size_t));
        co_await boost::asio::async_read(s, boost::asio::mutable_buffer(response_str.data() + sizeof(size_t), total_size), asio::use_awaitable);
        co_return response_str;
    }
};

}