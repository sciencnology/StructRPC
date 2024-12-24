#pragma once
#include <string>
#include "so_wrapper.h"


// 不支持函数重载和模板函数，函数返回类型必须是左值，输入参数类型可以是值或常量引用
class TCPTestServer
{
private:
    static constexpr std::string name = "TCPTestServer";

    template <typename F>
    constexpr std::string FunctionToName()
    {
        if constexpr (std::is_same_v<F, decltype(&echo)>) {
            return "echo";
        }
    }


    void RegisterServerFunction(TCPProcessCoroutineMap& process_coroutine_map, TCPProcessFuncMap& process_func_map)
    {
        if constexpr (trait_helper::function_helper::is_asio_coroutine<decltype(&echo)>) {
            process_coroutine_map[name][FunctionToName<decltype(&echo)>()] = [this](std::string_view input) -> asio::awaitable<std::string> { co_return co_await CommonFuncTemplate<decltype(&echo), true>(&echo, this, input); };
        } else {
            process_func_map[name][FunctionToName<decltype(&echo)>()] = [this](std::string_view input) -> std::string { return CommonFuncTemplate<decltype(&echo), false>(&echo, this, input); };
        }
    }

public:
    std::string echo(const std::string& input);
};

