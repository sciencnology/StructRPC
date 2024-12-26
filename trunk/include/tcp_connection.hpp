#pragma once

#include <utility>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/steady_timer.hpp>
#include <iostream>
#include <thread>
#include "nlohmann/json.hpp"
#include "common_define.hpp"
#include "trait_helper/trait_helper.hpp"


namespace struct_rpc{
using namespace std::chrono_literals;
using tcp = asio::ip::tcp;
class TCPConnectionBase
{
public:
    std::string host;
    std::string port;
    TCPConnectionBase(std::string host, std::string port): host(host), port(port) {}

    virtual std::string make_sync_tcp_request(std::string tcp_request) { throw std::runtime_error("not impled"); }
    virtual asio::awaitable<std::string> make_async_tcp_request(std::string tcp_request) { throw std::runtime_error("not impled"); }
    
    template <auto Func, typename... Args>
    auto sync_struct_rpc_request(Args&&... args) {
        using param_tuple_type = typename trait_helper::function_traits<decltype(Func)>::arguments_tuple;
        param_tuple_type param_tuple = std::make_tuple(std::forward<Args>(args)...);
        common_define::TCPRequest tcp_request {std::string(trait_helper::func_name<Func>()), structbuf::serializer::SaveToString(param_tuple)};
        std::string response_str = make_sync_tcp_request(structbuf::serializer::SaveToString(tcp_request));
        common_define::TCPResponse tcp_response;
        structbuf::deserializer::ParseFromSV(tcp_response, response_str);
        if (tcp_response.retcode != 0) {
            throw std::runtime_error("errcode" + std::to_string(tcp_response.retcode) + tcp_request.path);
        }
        using ReturnType = typename trait_helper::function_traits<decltype(Func)>::return_type;
        ReturnType function_return_obj;
        structbuf::deserializer::ParseFromSV(function_return_obj, tcp_response.data);

        return function_return_obj;
    }

    template <auto Func, typename... Args>
    auto async_struct_rpc_request(Args&&... args) 
        -> asio::awaitable<typename trait_helper::function_traits<decltype(Func)>::return_type>
    {
        using param_tuple_type = typename trait_helper::function_traits<decltype(Func)>::arguments_tuple;
        param_tuple_type param_tuple = std::make_tuple(std::forward<Args>(args)...);
        common_define::TCPRequest tcp_request {std::string(trait_helper::func_name<Func>()), structbuf::serializer::SaveToString(param_tuple)};
        std::string response_str = co_await make_async_tcp_request(structbuf::serializer::SaveToString(tcp_request));
        common_define::TCPResponse tcp_response;
        structbuf::deserializer::ParseFromSV(tcp_response, response_str);
        if (tcp_response.retcode != 0) {
            throw std::runtime_error("errcode" + std::to_string(tcp_response.retcode) + tcp_request.path);
        }
        using ReturnType = typename trait_helper::function_traits<decltype(Func)>::return_type;
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

    void connect()
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

    asio::awaitable<void> async_connect()
    {
        tcp::resolver resolver(s.get_executor());
        // auto co_await resolver.async_resolve(host, port, asio::use_awaitable);
        co_await asio::async_connect(s, resolver.resolve(host, port), asio::use_awaitable);
    }

    asio::awaitable<std::string> make_async_tcp_request(std::string tcp_request) override {
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