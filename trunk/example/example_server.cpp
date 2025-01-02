#include "functions.hpp"
#include <utility>
#include <iostream>
#include <thread>
#include <vector>

int main()
{
    TCPServer server(/* thread_num */ 2, /* listen_port */ 8080);
    server.RegisterServerFunctions<echo,  // 注册普通函数
        add,
        add_three,
        generic_add<int>, // 注册模板函数
        generic_add_various_params<int, int, double>, // 注册可变参数模板函数
        generic_add_various_params<int, int>,
        wait3s_and_echo,  // 注册coroutine
        ExampleRPCNamespace::add,   // 命名空间下的函数
        free_add_combined , // 注册自定义类型作为参数和返回值的函数
        &ExampleRPCClass::add,  // 注册类的成员函数，注意取成员函数指针时必须显式加&
        &ExampleRPCClass::static_add   // 注册静态成员函数
        // addo // 函数名拼写错误，可以在编译期检查并报错
        >();
    
    // run the server loop.
    server.Start();
    return 0;
}