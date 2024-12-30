#pragma once
#include "StructBuffer/struct_buffer.hpp"
#include "utils/trait_helper/trait_helper.hpp"
#include "utils/util.hpp"
#include <tuple>
#include <string_view>
#include <boost/asio.hpp>

namespace struct_rpc
{
    using namespace boost::asio;
    namespace asio = boost::asio;
    namespace common_define
    {
        /**
         * @brief: 通用的可用于序列化的模板数据类
         * @member data: 通过模板实例化可以存储任意类型和数量的合法序列化数据
        */
        template <typename... Args>
        struct CommonParamStruct
        {
            DEFINE_STRUCT_BUFFER_MEMBERS(data);

            std::tuple<Args...> data;
        };

        /**
         * @brief: RPC返回码
        */
        enum class RetCode
        {
            RET_SUCC = 0,
            RET_NOT_FOUND = 1,
            RET_SERVER_EXCEPTION = 2,
        };

        /**
         * @brief: TCP请求封装类
         * @member path: 请求的RPC函数路径，为通过function_name_getter自动提取的函数名
         * @member data: 请求参数列表按顺序组织成一个std::tuple后序列化成的字符串
        */
        struct TCPRequest
        {
            DEFINE_STRUCT_BUFFER_MEMBERS(path, data);

            std::string path;
            std::string data;
        };

        /**
         * @brief: TCP响应封装类
         * @member retcode: 返回码
         * @member data: RPC返回结果序列化成的字符串
        */
        struct TCPResponse
        {
            DEFINE_STRUCT_BUFFER_MEMBERS(retcode, data);

            int32_t retcode = 0;
            std::string data;
        };

    
        /**
         * @brief: 将所有协程类型的RPC处理函数类型擦除成function<awaitable<string>(string)>的形式
         * @param Func: 非类型模板参数，传入处理函数指针，针对每个函数会生成一份模板函数实例
         * @param input: 远程调用的请求参数列表按顺序组织成一个std::tuple后序列化成的字符串
         * @return: RPC调用结果序列化的字符串
        */
        template <auto Func>
        inline auto CommonCoroutineTemplate(std::string_view input) -> boost::asio::awaitable<std::string>
        {
            typename trait_helper::function_traits<decltype(Func)>::arguments_tuple input_struct;
            structbuf::deserializer::ParseFromSV(input_struct, input);
            if constexpr (trait_helper::is_member_function<decltype(Func)>) {
                using class_type = typename trait_helper::function_traits<decltype(Func)>::class_type;
                static_assert(!std::is_base_of_v<util::ThreadLocalSingleton<class_type>, class_type>, "you cannot use coroutine with ThreadLocalSingleton");
                auto ret = co_await std::apply(std::bind_front(Func, &class_type::getInstance()), input_struct);
                co_return structbuf::serializer::SaveToString(ret);
            } else {
                auto ret = co_await std::apply(Func, input_struct);
                co_return structbuf::serializer::SaveToString(ret);
            }
        }

        /**
         * @brief: 将所有普通RPC处理函数类型擦除成function<string(string)>的形式
         * @param Func: 非类型模板参数，传入处理函数指针，针对每个函数会生成一份模板函数实例
         * @param input: 远程调用的请求参数列表按顺序组织成一个std::tuple后序列化成的字符串
         * @return: RPC调用结果序列化的字符串
        */
        template <auto Func>
        inline auto CommonFuncTemplate(std::string_view input) -> std::string
        {
            // step 1. 提取函数的输入参数类型对应的tuple，并按照对应类型解析输入参数
            typename trait_helper::function_traits<decltype(Func)>::arguments_tuple input_struct;
            structbuf::deserializer::ParseFromSV(input_struct, input);

            // step 2. 使用std::apply将上面得到的tuple展开并传递给处理函数，返回序列化后的返回结果
            if constexpr (trait_helper::is_member_function<decltype(Func)>) {
                // note: 如果是成员函数，需要首先将对应的类对象绑定到第一个参数
                using class_type = typename trait_helper::function_traits<decltype(Func)>::class_type;
                auto ret = std::apply(std::bind_front(Func, &class_type::getInstance()), input_struct);
                return structbuf::serializer::SaveToString(ret);
            } else {
                auto ret = std::apply(Func, input_struct);
                return structbuf::serializer::SaveToString(ret);
            }
        }
    }
}





