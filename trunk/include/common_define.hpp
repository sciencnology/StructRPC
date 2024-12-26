#pragma once
#include "../StructBuffer/trunk/macros.h"
#include "../StructBuffer/trunk/serializer.hpp"
#include "../StructBuffer/trunk/deserializer.hpp"
#include "trait_helper/trait_helper.hpp"
#include <tuple>
#include <string_view>
#include <boost/asio.hpp>


namespace common_define
{
    template <typename... Args>
    struct CommonParamStruct
    {
        DEF_DATA_STUCT;
        DEFINE_GET_FUNC_1(data);

        std::tuple<Args...> data;
    };

    struct TCPRequest
    {
        DEF_DATA_STUCT;
        DEFINE_GET_FUNC_2(path, data);

        std::string path;
        std::string data;
    };

    struct TCPResponse
    {
        DEF_DATA_STUCT;
        DEFINE_GET_FUNC_2(retcode, data);

        int32_t retcode;
        std::string data;
    };

    enum class RetCode
    {
        RET_SUCC = 0,
        RET_NOT_FOUND = 1,
        RET_SERVER_EXCEPTION = 2,
    };

    template <auto Func>
    inline auto CommonCoroutineTemplate(std::string_view input) -> boost::asio::awaitable<std::string>
    {
        typename trait_helper::function_traits<decltype(Func)>::arguments_tuple input_struct;
        using class_type = typename trait_helper::function_traits<decltype(Func)>::class_type;
        structbuf::deserializer::ParseFromSV(input_struct, input);
        auto ret = co_await std::apply(std::bind_front(Func, &class_type::getInstance()), input_struct);

        // common_define::CommonParamStruct<decltype(ret)> output_struct{{std::move(ret)}};
        co_return structbuf::serializer::SaveToString(ret);
    }

    template <auto Func>
    inline auto CommonFuncTemplate(std::string_view input) -> std::string
    {
        typename trait_helper::function_traits<decltype(Func)>::arguments_tuple input_struct;
        using return_type = typename trait_helper::function_traits<decltype(Func)>::return_type;
        structbuf::deserializer::ParseFromSV(input_struct, input);
        return_type ret;
        if constexpr (trait_helper::is_member_function<decltype(Func)>) {
            using class_type = typename trait_helper::function_traits<decltype(Func)>::class_type;
            ret = std::apply(std::bind_front(Func, &class_type::getInstance()), input_struct);
        } else {
            ret = std::apply(Func, input_struct);
        }

        // common_define::CommonParamStruct<decltype(ret)> output_struct{{std::move(ret)}};
        return structbuf::serializer::SaveToString(ret);
    }
}




