#include "test_class.h"
#include "test_client.h"
#include <iostream>

asio::awaitable<void> Test::testNoRet(int a)
{
    std::cout << a << '\n';
    Test2Client client;
    client.set_port(8081);
    std::string ret = co_await client.appendString("abab");
    std::cout << ret << '\n';
    co_return;
}

asio::awaitable<int> Test::testRet()
{
    co_return 42;
}

asio::awaitable<void> Test::testStringInputParam(const std::string &a)
{
    std::cout << a << '\n';
    co_return;
}

asio::awaitable<void> Test::testStringOutputParam(std::string &a)
{
    a += "Modified";
    std::cout << a << '\n';
    co_return;
}

asio::awaitable<std::string> Test2::appendString(const std::string &a)
{
    std::cout << "in appendString" << std::endl;
    co_return a + "Modified";
}