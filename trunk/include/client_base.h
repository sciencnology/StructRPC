#ifndef CLIENT_BASE_H
#define CLIENT_BASE_H
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/steady_timer.hpp>
#include <iostream>
#include <thread>
#include "nlohmann/json.hpp"
#include "data_struct_define.hpp"
#include "trait_helper.hpp"

namespace http = boost::beast::http;
namespace asio = boost::asio;
namespace beast = boost::beast;
using namespace std;
using namespace std::chrono_literals;
using tcp = asio::ip::tcp;
using json = nlohmann::json;

// Helper macro to concatenate tokens
#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)

// Macro to declare a member variable and its setter function
#define DECLARE_MEMBER(type, name, default_val)     \
protected:                                          \
    type name = default_val;                        \
                                                    \
public:                                             \
    auto CONCATENATE(set_, name)(const type &input) \
    {                                               \
        this->name = input;                         \
        return *this;                               \
    }

class ClientBase
{
    DECLARE_MEMBER(std::string, host, "localhost");
    DECLARE_MEMBER(int, port, 8080);

public:
    asio::awaitable<json> make_request(std::string target, json &params);

    asio::awaitable<std::string> make_tcp_request(std::string target, std::string tcp_request);

    template <typename F, typename... Args>
    auto do_remote_request(std::string class_name, std::string method, F&& f, Args&& args...)
    {
        TCPRequest tcp_request {class_name, method, std::make_tuple(args...)};
        std::string response_str = co_await make_tcp_request(host, structbuf::serializer::SaveToString(tcp_request));
        TCPResponse tcp_response;
        structbuf::deserializer::ParseFromSV(tcp_response, response_str);
        if (tcp_response.retcode != 0) {
            throw std::runtime_error("errcode");
        }
        using ReturnType = typename trait_helper::function_helper::function_traits<F>::return_type;
        ReturnType function_return_obj;
        structbuf::deserializer::ParseFromSV(function_return_obj, tcp_response.data);
        co_return function_return_obj;
    }
};

#endif
