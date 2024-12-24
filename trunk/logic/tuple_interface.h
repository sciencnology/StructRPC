#pragma once
#include "../StructBuffer/trunk/macros.h"
#include "../StructBuffer/trunk/serializer.hpp"
#include "../StructBuffer/trunk/deserializer.hpp"
#include "trait_helper/func_name.hpp"
#include "trait_helper/func_traits.hpp"
#include "trait_helper/partial_apply.hpp"
#include "data_struct_define.hpp"
#include <tuple>
#include <string_view>
#include <boost/asio.hpp>

namespace TupleInterface
{
    template <auto Func>
    inline auto CommonCoroutineTemplate(std::string_view input) -> boost::asio::awaitable<std::string>
    {
        typename trait_helper::function_helper::function_traits<decltype(Func)>::arguments_tuple input_struct;
        using class_type = typename trait_helper::function_helper::function_traits<decltype(Func)>::class_type;
        using return_type = typename trait_helper::function_helper::function_traits<decltype(Func)>::return_type;
        structbuf::deserializer::ParseFromSV(input_struct, input);
        auto ret = co_await std::apply(std::bind_front(Func, &class_type::getInstance()), input_struct);

        CommonParamStruct<decltype(ret)> output_struct{{std::move(ret)}};

        co_return structbuf::serializer::SaveToString(output_struct);
    }

    template <auto Func>
    inline auto CommonFuncTemplate(std::string_view input) -> std::string
    {
        typename trait_helper::function_helper::function_traits<decltype(Func)>::arguments_tuple input_struct;
        using class_type = typename trait_helper::function_helper::function_traits<decltype(Func)>::class_type;
        structbuf::deserializer::ParseFromSV(input_struct, input);
        
        auto ret = std::apply(std::bind_front(Func, &class_type::getInstance()), input_struct);

        CommonParamStruct<decltype(ret)> output_struct{{std::move(ret)}};

        return structbuf::serializer::SaveToString(output_struct);
    }


}
