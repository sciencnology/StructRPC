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
};

#endif
