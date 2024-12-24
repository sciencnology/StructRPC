#pragma once
#include <string>
#include <unordered_map>
#include "so_wrapper.h"
#include "trait_helper/func_name.hpp"
#include "trait_helper/func_traits.hpp"


// 不支持函数重载、模板函数、静态函数和虚函数，函数输入参数和返回类型都必须是左值，不可以是引用
class TCPTestServer : public util::Singleton<TCPTestServer>
{
public:
    std::string echo(std::string input) {
        return input;
    }

    int32_t add(int32_t a, int32_t b) {
        return a + b;
    }

    boost::asio::awaitable<int> coro(int i) {
        co_return i;
    }
};

