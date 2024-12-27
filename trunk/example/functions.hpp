#pragma once
#include <string>
#include "../struct_rpc.hpp"

using namespace struct_rpc;

/**
 * 支持注册类的成员函数，但是要求对应的类必须是单例类，即派生自以下基类之一：
 * * util::Singleton<T> 全局单例，所有server线程共享同一个实例，需要注意线程同步问题
 * * util::ThreadLocalSingleton<T> 线程局部单例，每个server线程维护一个实例
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

    /**
     * 另一个普通成员函数
    */
    int32_t add(int32_t a, int32_t b) {
        return a + b;
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
     * 甚至支持可变数量模板参数
    */
    template<typename... Args>
    std::common_type_t<Args...> generic_add_various_params(Args... args) {
        return (args + ...);
    }

    /**
     * 支持静态成员函数
    */
    static int static_add(int a, int b) {
        return a + b;
    }

    /**
     * 支持基于boost::asio的C++20协程
    */
    awaitable<int> coro(int i) {
        co_return i;
    }

    /** 支持函数重载，但是注册和调用时需要使用特殊语法，本处不做展示
    * int32_t echo(int32_t input) {
    *     return input;
    * }
    */

   /**
    * 只支持按值传参并按值返回结果，不支持按引用传参和void返回类型
    * void add_ref_args(int a, int b, int& c) {
    *    c = a + b;
    * }
   */
    
};

/**
 * 支持free-function
*/
inline int32_t free_add(int32_t a, int32_t b) {
    return a + b;
}

namespace ExampleRPCNamespace {
    inline int32_t add(int32_t a, int32_t b) {
        return a + b;
    }
}