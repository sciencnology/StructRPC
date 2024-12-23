#include "so_wrapper.h"
#include "test_server.h"
#include "tuple_interface.h"
#include "client_base.h"
#include <boost/asio/awaitable.hpp>

void init_process_map(ProcessFuncMap &func_map, ProcessCoroutineMap &coroutine_map)
{
    coroutine_map["Test"]["testRet"] = [](json &params) -> asio::awaitable<std::string>
    {
        std::string ret = co_await TestInterface::testRet(params);
        co_return ret;
    };

    coroutine_map["Test"]["testStringInputParam"] = std::bind(TestInterface::testStringInputParam, std::placeholders::_1);
    coroutine_map["Test"]["testNoRet"] = std::bind(TestInterface::testNoRet, std::placeholders::_1);

    coroutine_map["Test2"]["appendString"] = std::bind(Test2Interface::appendString, std::placeholders::_1);

    coroutine_map["abab"]["abc"] = [](json &params) -> asio::awaitable<std::string>
    {
        co_return params.dump();
    };

    coroutine_map["abab"]["ab"] = [](json &params) -> asio::awaitable<std::string>
    {
        ClientBase client;
        client.set_host("localhost");
        client.set_port(8080);
        json param;
        param["test"] = "test";
        // asio::co_spawn(asio::this_coro::executor, client.make_request("/abab/abc", param), asio::detached);
        co_return (co_await client.make_request("/Test2/appendString", param)).dump();
    };
}

// 不支持函数重载
void init_tcp_process_map(ProcessFuncMap &func_map, TCPProcessCoroutineMap &coroutine_map)
{
    coroutine_map["Test"]["testStringOutputParam"] = [](std::string_view input) -> asio::awaitable<std::string>
    {
        co_return co_await TupleInterface::CommonFuncTemplate(&TestInterface::testTCP, input);
    };
}