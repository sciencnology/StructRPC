#pragma once
#include "../StructBuffer/trunk/macros.h"
#include "../StructBuffer/trunk/serializer.hpp"
#include "../StructBuffer/trunk/deserializer.hpp"
#include "trait_helper.hpp"
#include <tuple>
#include <string_view>
#include <boost/asio.hpp>

namespace TupleInterface
{
    

    template <typename F, bool IsCoroutine = true>
    inline auto CommonFuncTemplate(F &&f, std::string_view input)
    {

        typename trait_helper::function_helper::function_traits<F>::arguments_tuple input_struct;
        structbuf::deserializer::ParseFromSV(input_struct, input);
        // auto ret = co_await std::apply([f = std::forward<F>(f)](auto &&...args)
        //                                { co_return co_await f(args...); }, input_struct.data);
        if constexpr (IsCoroutine) {
            auto ret = co_await trait_helper::function_helper::apply_from_tuple(f, input_struct);
        } else {
            auto ret = trait_helper::function_helper::apply_from_tuple(f, input_struct);
        }
        
        CommonParamStruct<decltype(ret)> output_struct{{std::move(ret)}};
        if constexpr (IsCoroutine) {
            co_return structbuf::serializer::SaveToString(output_struct);
        } else {
            return structbuf::serializer::SaveToString(output_struct);
        }
        
    }
}
