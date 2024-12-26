#pragma once
#ifndef CLIENT_BASE_H
#define CLIENT_BASE_H
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

namespace http = boost::beast::http;
namespace asio = boost::asio;
namespace beast = boost::beast;
using namespace std;
using namespace std::chrono_literals;
using tcp = asio::ip::tcp;
using json = nlohmann::json;


enum class RequestMode
{
    SYNC_REQUEST,
    ASYNC_REQUEST,
    QSYNC_REQUEST_WITH_CONN_POOL,
};


template <RequestMode Mode = RequestMode::SYNC_REQUEST>
inline std::string make_tcp_request(std::string host, std::string port, std::string tcp_request)
{

    common_define::TCPRequest req_obj;
    structbuf::deserializer::ParseFromSV(req_obj, tcp_request);









    boost::asio::io_context io_context;

    tcp::socket s(io_context);
    tcp::resolver resolver(io_context);
    boost::asio::connect(s, resolver.resolve(host, port));
    boost::asio::write(s, boost::asio::buffer(tcp_request, tcp_request.size()));
    size_t total_size;
    boost::asio::read(s, boost::asio::buffer(&total_size, sizeof(size_t)));
    std::string response_str(total_size + sizeof(size_t), '\0');
    std::memcpy(response_str.data(), &total_size, sizeof(size_t));
    boost::asio::read(s, boost::asio::buffer(response_str.data() + sizeof(size_t), total_size));
    return response_str;
}

template <RequestMode Mode, auto Func, typename... Args>
inline auto do_remote_request(std::string host, std::string port, Args&&... args) 
    -> trait_helper::conditional_template_t<Mode != RequestMode::SYNC_REQUEST, asio::awaitable, typename trait_helper::function_traits<decltype(Func)>::return_type>
{
    std::string path = std::string(trait_helper::func_name<Func>());
    using param_tuple_type = typename trait_helper::function_traits<decltype(Func)>::arguments_tuple;
    param_tuple_type param_tuple {args...};
    // common_define::TCPRequest tcp_request {path, structbuf::serializer::SaveToString(std::make_tuple(std::forward<Args...>(args...)))};
    common_define::TCPRequest tcp_request {path, structbuf::serializer::SaveToString(param_tuple)};
    std::string response_str;
    if constexpr (Mode == RequestMode::SYNC_REQUEST) {
        response_str = make_tcp_request<Mode>(host, port, structbuf::serializer::SaveToString(tcp_request));
    } else {
        // response_str = co_await make_tcp_request<Mode>(host, port, structbuf::serializer::SaveToString(tcp_request));
    }
    common_define::TCPResponse tcp_response;
    structbuf::deserializer::ParseFromSV(tcp_response, response_str);
    if (tcp_response.retcode != 0) {
        throw std::runtime_error("errcode");
    }
    using ReturnType = typename trait_helper::function_traits<decltype(Func)>::return_type;
    ReturnType function_return_obj;
    structbuf::deserializer::ParseFromSV(function_return_obj, tcp_response.data);
    if constexpr (Mode == RequestMode::SYNC_REQUEST) {
        return function_return_obj;
    } else {
        // co_return function_return_obj;
    } 
}


#endif
