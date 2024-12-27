#pragma once
#include <string>
#include <string_view>
#include <array>   // std::array
#include <utility> // std::index_sequence
#include <iostream>
#include "type_name_getter.hpp"

namespace struct_rpc
{
namespace trait_helper
{

    template <auto Addr>
    constexpr auto func_name_array()
    {
        #if defined(__clang__)
            constexpr auto prefix   = std::string_view{"[Addr = "};
            constexpr auto suffix   = std::string_view{"]"};
            constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
        #elif defined(__GNUC__)
            constexpr auto prefix   = std::string_view{"with auto Addr = "};
            constexpr auto suffix   = std::string_view{"]"};
            constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
        #elif defined(_MSC_VER)
            // constexpr auto prefix   = std::string_view{"func_name_array<"};
            // constexpr auto suffix   = std::string_view{">(void)"};
            // constexpr auto function = std::string_view{__FUNCSIG__};
            # error Unsupported compiler MSVC
        #else
            # error Unsupported compiler
        #endif

        constexpr auto start = function.find(prefix) + prefix.size();
        constexpr auto end = function.rfind(suffix);

        static_assert(start < end);

        constexpr auto name = function.substr(start, (end - start));
        return substring_as_array(name, std::make_index_sequence<name.size()>{});
    }

    template <auto Addr>
    struct func_name_holder {
        static inline constexpr auto value = func_name_array<Addr>();
    };

    template <auto Addr>
    constexpr auto func_name()
    {
        constexpr auto& value = func_name_holder<Addr>::value;
        return value;
    }


    template <typename Type, std::size_t... sizes>
    constexpr auto concatenate_arrays(const std::array<Type, sizes>&... arrays)
    {
        std::array<Type, (sizes + ...)> result;
        std::size_t index{};

        ((std::copy_n(arrays.begin(), sizes, result.begin() + index), index += sizes), ...);

        return result;
    }

    template <auto Addr>
    struct struct_rpc_func_path_holder {
        static inline constexpr std::array<char, 2> splitter {'-', '-'};
        static inline constexpr auto value = concatenate_arrays(func_name<Addr>(), splitter, get_type_name<decltype(Addr)>());
    };

    template <auto Addr>
    constexpr auto struct_rpc_func_path() -> std::string_view
    {
        constexpr auto& value = struct_rpc_func_path_holder<Addr>::value;
        return std::string_view(value.data(), value.size());
    }
}

}