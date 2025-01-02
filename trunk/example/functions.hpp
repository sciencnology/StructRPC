#pragma once
#include <string>
#include "../struct_rpc.hpp"

using namespace struct_rpc;

/**
 * 支持普通的free-function
*/
inline int32_t add(int32_t a, int32_t b) {
    return a + b;
}
inline std::string echo(std::string input) {
    return input;
}

/**
 * 支持任意数量的函数参数
*/
int32_t add_three(int32_t a, int32_t b, int c) {
    return a + b + c;
}

/**
 * 支持模板函数，但是要注意不同模板实例的函数签名类型必须不同
*/
template<typename T>
T generic_add(T a, T b) {
    return a + b;
}

/**
 * 支持可变数量模板参数
*/
template<typename... Args>
std::common_type_t<Args...> generic_add_various_params(Args... args) {
    return (args + ...);
}

/**
 * 支持基于boost::asio的C++20协程,e.g.:
*/
awaitable<int> wait3s_and_echo(int i) {
    steady_timer timer(co_await this_coro::executor);
    timer.expires_after(std::chrono::seconds(3));
    co_await timer.async_wait(use_awaitable);
    co_return i;
}

/**
 * 函数参数和返回值支持自定义结构体，需要使用DEFINE_STRUCT_BUFFER_MEMBERS宏注册成StructBuffer可解析的形式
*/
struct CombinedStruct {
    DEFINE_STRUCT_BUFFER_MEMBERS(str_member, int_member);
    std::string str_member;
    uint32_t int_member;
};
inline CombinedStruct free_add_combined(CombinedStruct a, CombinedStruct b) {
    return CombinedStruct{a.str_member + b.str_member, a.int_member + b.int_member};
}

/** 支持函数重载，但是注册和调用时需要使用特殊语法，本处不做展示
* int32_t echo(int32_t input) {
*     return input;
* }
*/

/**
* 只支持按值传参并按值返回结果，不支持按引用传参和void返回类型，例如下面的函数是不支持的：
* void add_ref_args(int a, int b, int& c) {
*    c = a + b;
* }
*/

namespace ExampleRPCNamespace {
    /**
     * 支持namespace下的函数
    */
    inline int32_t add(int32_t a, int32_t b) {
        return a + b;
    }
}

/**
 * 支持注册类的成员函数，但是目前要求对应的类必须是单例类，即派生自以下基类之一：
 * * util::Singleton<T> 全局单例，所有server线程共享同一个实例，需要注意线程同步问题
 * * util::ThreadLocalSingleton<T> 线程局部单例，每个server线程维护一个实例
 * * 或者需要实现一个静态方法getInstance用于获取对象实例。
*/
class ExampleRPCClass : public util::Singleton<ExampleRPCClass>
{
public:
    /**
     * 支持普通成员函数
    */
    std::string echo(std::string input) {
        return input;
    }

    int32_t add(int32_t a, int32_t b) {
        return a + b;
    }

    /**
     * 支持静态成员函数
    */
    static int static_add(int a, int b) {
        return a + b;
    }
};