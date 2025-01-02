#include "functions.hpp"
#include <utility>
#include <iostream>
#include <thread>
#include <vector>

int main()
{
    TCPServer server(/* thread_num */ 2, /* listen_port */ 8080);
    server.RegisterServerFunctions<&ExampleRPCClass::echo,  // 注册类的普通成员函数
        &ExampleRPCClass::add,
        &ExampleRPCClass::add_three,
        &ExampleRPCClass::generic_add<int>, // 注册模板函数
        &ExampleRPCClass::generic_add_various_params<int, int, double>, // 注册可变参数模板函数
        &ExampleRPCClass::generic_add_various_params<int, int>,
        &ExampleRPCClass::static_add,   // 注册静态成员函数
        &ExampleRPCClass::wait3s_and_echo,  // 注册coroutine
        &free_add,  // 注册非成员函数
        // &free_addo, // 函数名拼写错误，可以在编译期检查并报错
        &ExampleRPCNamespace::add,
        &free_add_combined  // 注册自定义类型作为参数和返回值的函数
        >();
    
    // run the server loop.
    server.Start();
    return 0;
}