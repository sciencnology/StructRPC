#pragma once
#include <type_traits>
#include <vector>
#include <functional>

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
        };

        // const 成员函数
        template <typename R, typename C, typename... Args>
        struct function_traits<R (C::*)(Args...) const>
        {
            using return_type = R;
            using arguments_tuple = std::tuple<Args...>;
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


        template <typename F>
        inline constexpr is_asio_coroutine = trait_helper::is_specialization_of_v<asio::awaitable, typename function_traits<F>::return_type>;

    }
}