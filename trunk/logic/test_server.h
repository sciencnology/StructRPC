#ifndef TEST_SERVER_H
#define TEST_SERVER_H
#include "test_class.h"
#include "nlohmann/json.hpp"
#include <boost/asio.hpp>
using json = nlohmann::json;
namespace asio = boost::asio;

/**
 * auto generate, do not modify
 */

class TestInterface
{
public:
    static asio::awaitable<std::string> testNoRet(json &params)
    {
        co_await Test::getInstance().testNoRet(params["a"]);
        json j;
        j["Params"] = params;
        co_return j.dump();
    }
    static asio::awaitable<std::string> testRet(json &params)
    {
        int ret = co_await Test::getInstance().testRet();
        json j;
        j["Ret"] = ret;
        j["Params"] = params;
        co_return j.dump();
    }
    static asio::awaitable<std::string> testStringInputParam(json &params)
    {
        co_await Test::getInstance().testStringInputParam(params["a"]);
        json j;
        j["Ret"] = {};
        j["Params"] = params;
        co_return j.dump();
    }
    static asio::awaitable<std::string> testStringOutputParam(json &params)
    {
        std::string a = params["a"];
        co_await Test::getInstance().testStringInputParam(a);
        params["a"] = a;
        json j;
        j["Ret"] = {};
        j["Params"] = params;
        co_return j.dump();
    }

    static asio::awaitable<std::string> testTCP(int a, double b)
    {
        
        co_return "dsadas";
    }
};

class Test2Interface
{
public:
    static asio::awaitable<std::string> appendString(json &params)
    {
        std::string ret = co_await Test2::getInstance().appendString(params["a"]);
        json j;
        j["Params"] = params;
        j["Ret"] = ret;
        co_return j.dump();
    }
};

#endif // TEST_SERVER_H