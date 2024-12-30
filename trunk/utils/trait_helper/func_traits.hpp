#pragma once
#include <type_traits>
#include <vector>
#include <functional>
#include <boost/asio.hpp>
#include "../../StructBuffer/trunk/utils/trait_helper.h"
namespace struct_rpc
{
namespace trait_helper
{
    /**
     * @brief: 函数特征萃取，可以提取函数的返回类型、输入参数类型列表、成员函数的类类型（如有）
    */
    template <typename>
    struct function_traits;

    // specialization for 普通函数指针
    template <typename R, typename... Args>
    struct function_traits<R (*)(Args...)>
    {
        using return_type = R;
        using arguments_tuple = std::tuple<Args...>;
    };

    // specialization for 普通函数
    template <typename R, typename... Args>
    struct function_traits<R(Args...)>
    {
        using return_type = R;
        using arguments_tuple = std::tuple<Args...>;
    };

    // specialization for 成员函数
    template <typename R, typename C, typename... Args>
    struct function_traits<R (C::*)(Args...)>
    {
        using return_type = R;
        using arguments_tuple = std::tuple<Args...>;
        using class_type = C;
    };

    // specialization for const 成员函数
    template <typename R, typename C, typename... Args>
    struct function_traits<R (C::*)(Args...) const>
    {
        using return_type = R;
        using arguments_tuple = std::tuple<Args...>;
        using class_type = C;
    };

    // specialization for std::function
    template <typename R, typename... Args>
    struct function_traits<std::function<R(Args...)>>
    {
        using return_type = R;
        using arguments_tuple = std::tuple<Args...>;
    };

    // specialization for Lambda
    template <typename T>
    struct function_traits : function_traits<decltype(&T::operator())>
    {
    };


    /**
     * @brief: 判断一个函数是否为asio的协程类型
     * @note: 实际上判断的是函数返回类型是否是boost::asio::awaitable的实例
    */
    template <typename F>
    concept is_asio_coroutine = structbuf::trait_helper::is_specialization_of_v<typename function_traits<F>::return_type, boost::asio::awaitable>;

    /**
     * @brief: 判断一个函数指针是否为某个类的成员函数
    */
    template <typename F>
    concept is_member_function = requires { typename function_traits<F>::class_type; };

    template <typename F>
    struct rpc_return_type_getter {
        using type = typename function_traits<F>::return_type;
    };

    template <typename F>
        requires is_asio_coroutine<F>
    struct rpc_return_type_getter<F> {
        using type = typename function_traits<F>::return_type::value_type;
    };

    // template <typename F>
    // using rpc_return_type_getter_v = typename rpc_return_type_getter::type;

}
}