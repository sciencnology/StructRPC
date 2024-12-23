#ifndef TEST_H
#define TEST_H
#include <cstdlib>
#include <string>
#include <boost/asio.hpp>
#include "util.hpp"
namespace asio = boost::asio;

class Test : public util::Singleton<Test>
{
public:
    asio::awaitable<void> testNoRet(int a);
    asio::awaitable<int> testRet();
    asio::awaitable<void> testStringInputParam(const std::string &a);
    asio::awaitable<void> testStringOutputParam(std::string &a);
};

class Test2 : public util::Singleton<Test2>
{
public:
    asio::awaitable<std::string> appendString(const std::string &a);
};

#endif // TEST_H