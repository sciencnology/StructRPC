#pragma once
#include <string>
#include <unordered_map>
#include <iostream>
// #include "tcp_server.hpp"
// #include "util.hpp"
// #include "trait_helper/trait_helper.hpp"
#include "../struct_rpc.hpp"

using namespace struct_rpc;

// 不支持函数重载、模板函数、静态函数和虚函数，函数输入参数和返回类型都必须是左值，不可以是引用
class TCPTestServer : public util::Singleton<TCPTestServer>
{
public:
    std::string echo(std::string input) {
        return input;
    }

    // 不支持普通函数或者类的成员函数的重载，请使用其他方式
    // int32_t echo(int32_t input) {
    //     return input;
    // }

    int32_t add(int32_t a, int32_t b) {
        return a + b;
    }

    boost::asio::awaitable<int> coro(int i) {
        co_return i;
    }

    template<typename T>
    T generic_add(T a, T b) {
        return a + b;
    }

    template<typename... Args>
    std::common_type_t<Args...> generic_add_various_params(Args... args) {
        return (args + ...);
    }

    static int static_sub(int a, int b) {
        return a - b;
    }
};


int32_t free_add(int32_t a, int32_t b) {
    return a + b;
}

