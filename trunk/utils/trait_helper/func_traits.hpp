#pragma once
#include <type_traits>
#include <vector>
#include <functional>
#include <boost/asio.hpp>
#include "../../StructBuffer/trunk/utils/trait_helper.h"

namespace trait_helper
{
    namespace function_helper
    {
        // 通用模板类
        template <typename>
        struct function_traits;

        // 普通函数指针
        template <typename R, typename... Args>
        struct function_traits<R (*)(Args...)>
        {
            using return_type = R;
            using arguments_tuple = std::tuple<Args...>;
        };

        // 普通函数
        template <typename R, typename... Args>
        struct function_traits<R(Args...)>
        {
            using return_type = R;
            using arguments_tuple = std::tuple<Args...>;
        };

        // 成员函数
        template <typename R, typename C, typename... Args>
        struct function_traits<R (C::*)(Args...)>
        {
            using return_type = R;
            using arguments_tuple = std::tuple<Args...>;
            using class_type = C;
        };

        // const 成员函数
        template <typename R, typename C, typename... Args>
        struct function_traits<R (C::*)(Args...) const>
        {
            using return_type = R;
            using arguments_tuple = std::tuple<Args...>;
            using class_type = C;
        };

        // std::function
        template <typename R, typename... Args>
        struct function_traits<std::function<R(Args...)>>
        {
            using return_type = R;
            using arguments_tuple = std::tuple<Args...>;
        };

        // Lambda (通过 operator())
        template <typename T>
        struct function_traits : function_traits<decltype(&T::operator())>
        {
        };



        template <typename F, typename Tuple, size_t... S>
        decltype(auto) apply_tuple_impl(F &&fn, Tuple &&t, std::index_sequence<S...>)
        {
            return std::forward<F>(fn)(std::get<S>(std::forward<Tuple>(t))...);
        }
        template <typename F, typename Tuple>
        decltype(auto) apply_from_tuple(F &&fn, Tuple &&t)
        {
            std::size_t constexpr tSize = std::tuple_size<typename std::remove_reference<Tuple>::type>::value;
            return apply_tuple_impl(std::forward<F>(fn),
                                    std::forward<Tuple>(t),
                                    std::make_index_sequence<tSize>());
        }


        template <typename F, typename Tuple, size_t... S>
        typename function_traits<F>::return_type apply_tuple_to_coroutine_impl(F &&fn, Tuple &&t, std::index_sequence<S...>)
        {
            co_return co_await std::forward<F>(fn)(std::get<S>(std::forward<Tuple>(t))...);
        }

        template <typename F, typename Tuple>
        typename function_traits<F>::return_type apply_from_tuple_to_coroutine(F &&fn, Tuple &&t)
        {
            std::size_t constexpr tSize = std::tuple_size<typename std::remove_reference<Tuple>::type>::value;
            co_return co_await apply_tuple_impl(std::forward<F>(fn),
                                    std::forward<Tuple>(t),
                                    std::make_index_sequence<tSize>());
        }


        template <typename F>
        inline constexpr bool is_asio_coroutine = structbuf::trait_helper::is_specialization_of_v<typename function_traits<F>::return_type, boost::asio::awaitable>;

    }
}