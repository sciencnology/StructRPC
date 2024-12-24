#pragma once
#include <type_traits>
#include <tuple>
#include <functional>

namespace trait_helper
{
    template <typename F, typename... Args>
    class partial_t {
    public:
        constexpr partial_t(F&& f, Args&&... args) : f_(std::forward<F>(f)),
            args_(std::forward_as_tuple(args...)) {
        }

        template <typename... RestArgs>
        constexpr decltype(auto) operator()(RestArgs&&... rest_args) {
            return std::apply(f_, std::tuple_cat(args_,
                std::forward_as_tuple(std::forward<RestArgs>(rest_args)...)));
        }

    private:
        F f_;
        std::tuple<Args...> args_;
    };

    template <typename Fn, typename... Args>
    constexpr decltype(auto) partial(Fn&& fn, Args&&... args) {
        return partial_t<Fn, Args...>(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
}

