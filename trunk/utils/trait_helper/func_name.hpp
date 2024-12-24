#pragma once
#include <string>
#include <string_view>
#include <array>   // std::array
#include <utility> // std::index_sequence
#include <iostream>


namespace trait_helper
{
    template <std::size_t...Idxs>
    constexpr auto substring_as_array(std::string_view str, std::index_sequence<Idxs...>)
    {
        return std::array{str[Idxs]..., '\n'};
    }

    template <auto Addr>
    constexpr auto func_name_array()
    {
        #if defined(__clang__)
            constexpr auto prefix   = std::string_view{"[Addr = &"};
            constexpr auto suffix   = std::string_view{"]"};
            constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
        #elif defined(__GNUC__)
            constexpr auto prefix   = std::string_view{"with auto Addr = &"};
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
    constexpr auto func_name() -> std::string_view
    {
        constexpr auto& value = func_name_holder<Addr>::value;
        return std::string_view{value.data(), value.size()};
    }


}

