# StructRPC

## Overview

​`StructRPC`​是一个高用户友好度、高性能、高扩展性的C++RPC库，提供了基于boost asio和C++20 Coroutine实现的TCP Server和Client。

本项目致力于在保证性能的前提下提供最接近原生函数调用的RPC体验，使用少量代码即可轻松地在编译期将各种类型的C++函数定义注册到RPC Server中，并在Client处指定特定函数，传入任意合法的函数参数进行远程调用。

`StructRPC`支持如下几种类型的C++函数进行RPC注册和调用：
* 全局及命名空间下的free-function
* 模板及可变参数模板函数
* 类的普通和静态成员函数（暂不支持虚函数）
* 重载函数
* 按值传参或按引用传参皆可。如果函数按引用传参，则调用方可以同步得到函数对参数的修改。如果函数有非void返回值，则调用方可以得到函数的返回值。
‍

## How to Use
* 本项目git目录中包含submodule StructBuffer，需要使用 `git clone --recurse-submodules https://github.com/sciencnology/StructRPC` 进行拉取。

* 本项目为header-only实现，只依赖Boost Asio，直接在代码中`#include "StructRPC/trunk/struct_rpc.hpp"`​即可使用
* C++语言标准：C++20  Boost版本： 1.80.0。开发和测试使用的编译器为gcc13.2。理论上使用gcc10及以上的版本即可编译，参考https://en.cppreference.com/w/cpp/compiler_support/20

‍

## Examples

### Common

下面展示一些可用于注册远程调用的C++函数。由于RPC函数的注册和调用需要使用函数指针，RPC Server处需要有函数的正确定义，Client处可以只给出默认的空定义（如果一个函数只有声明而无定义则无法取地址）：

```c++
#pragma once
#include <string>
#include "../struct_rpc.hpp"

using namespace struct_rpc;

/**
 * 支持普通的free-function
*/
inline int32_t add(int32_t a, int32_t b) {
    return a + b;
}
inline std::string echo(std::string input) {
    return input;
}

/**
 * 支持任意数量的函数参数
*/
int32_t add_three(int32_t a, int32_t b, int c) {
    return a + b + c;
}

/**
 * 支持按引用方式传递参数，及void返回类型。函数对引用参数的修改也会同步到请求端。
*/
inline void add_ref(int a, int b, int& c) {
    c = a + b;
}

/**
 * 支持模板函数，但是要注意不同模板实例的函数签名类型必须不同
*/
template<typename T>
T generic_add(T a, T b) {
    return a + b;
}

/**
 * 支持可变数量模板参数
*/
template<typename... Args>
std::common_type_t<Args...> generic_add_various_params(Args... args) {
    return (args + ...);
}

/**
 * 支持基于boost::asio的C++20协程,e.g.:
*/
awaitable<int> wait3s_and_echo(int i) {
    steady_timer timer(co_await this_coro::executor);
    timer.expires_after(std::chrono::seconds(3));
    co_await timer.async_wait(use_awaitable);
    co_return i;
}

/**
 * 函数参数和返回值支持自定义结构体
*/
struct CombinedStruct {
    std::string str_member;
    uint32_t int_member;
};
inline CombinedStruct free_add_combined(CombinedStruct a, CombinedStruct b) {
    return CombinedStruct{a.str_member + b.str_member, a.int_member + b.int_member};
}

/** 支持函数重载，但是注册和调用时需要使用特殊语法，本处不做展示
* int32_t echo(int32_t input) {
*     return input;
* }
*/

namespace ExampleRPCNamespace {
    /**
     * 支持namespace下的函数
    */
    inline int32_t add(int32_t a, int32_t b) {
        return a + b;
    }
}

/**
 * 支持注册类的成员函数，但是目前要求对应的类必须是单例类，即派生自以下基类之一：
 * * util::Singleton<T> 全局单例，所有server线程共享同一个实例，需要注意线程同步问题
 * * util::ThreadLocalSingleton<T> 线程局部单例，每个server线程维护一个实例
 * * 或者需要实现一个静态方法getInstance用于获取对象实例。
*/
class ExampleRPCClass : public util::Singleton<ExampleRPCClass>
{
public:
    /**
     * 支持普通成员函数
    */
    std::string echo(std::string input) {
        return input;
    }

    int32_t add(int32_t a, int32_t b) {
        return a + b;
    }

    /**
     * 支持静态成员函数
    */
    static int static_add(int a, int b) {
        return a + b;
    }
};
```

### Server

```c++
// example_server.cpp
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
        &ExampleRPCClass::static_add,  // 注册静态成员函数
        // addo // 函数名拼写错误，可以在编译期检查并报错
        add_ref // 注册按引用传参并返回void的函数
        >();
    
    // 启动server循环，会阻塞当前线程，并在内部开启多线程异步处理请求。
    server.Start();
    return 0;
}
```

### Client

由于篇幅问题，这里只展示同步调用方式，异步调用可参考`StructRPC/trunk/example`​中的示例。

```c++
// sync_client.cpp
#include "functions.hpp"
#include <format>
#include <memory>
using std::cout;
using std::endl;
using std::format;

int main()
{
    std::unique_ptr<TCPConnectionBase> conn = std::make_unique<SyncTCPConnection>("127.0.0.1", "8080");
    cout << conn->sync_struct_rpc_request<echo>("thisisateststring") << endl;  // 调用普通函数，返回thisisateststring
    cout << conn->sync_struct_rpc_request<add>(1, 2) << endl;  // 3
    cout << conn->sync_struct_rpc_request<generic_add<int>>(1, 2) << endl; // 调用模板函数，返回3
    cout << conn->sync_struct_rpc_request<wait3s_and_echo>(1) << endl; // 调用协程，阻塞三秒后返回1
    cout << conn->sync_struct_rpc_request<generic_add_various_params<int, int, double>>(1, 2, 3.5) << endl;    // 调用可变参数模板函数，返回6.5
    cout << conn->sync_struct_rpc_request<generic_add_various_params<int, int>>(10, 10) << endl;   // 20
    // cout << conn->sync_struct_rpc_request<addo>(10, 10) << endl;    // 函数名拼写错误，可以在编译期检查并报错
    cout << conn->sync_struct_rpc_request<free_add_combined>(
        CombinedStruct{"hello ", 1}, 
        CombinedStruct{"world", 2}).str_member << endl;   // 调用接收复杂类型参数的函数，返回{"hello world", 3}
    cout << conn->sync_struct_rpc_request<&ExampleRPCClass::add>(10, 10) << endl;    // 调用类的成员函数，返回20
    
    int c = 0;
    conn->sync_struct_rpc_request<add_ref>(1, 2, c);
    cout << c << endl;  // 调用按引用传参的函数，返回3

    return 0;
}
```

‍

## Advantages

* 相比BRPC、GRPC、rpclib等其他C++ RPC框架，`StructRPC`​在易用性上有很大改进，包括但不限于：

  * BRPC、GRPC等框架需要针对每个RPC函数的参数和返回值声明对应的proto message，且需要经过protoc代码生成才可在项目中使用。`StructRPC`​使用可变参数模板封装RPC调用过程，调用者直接传入全部参数即可调用，且不需要任何代码生成/二次编译步骤。
  * ​`StructRPC`​的函数注册和调用全部为编译期绑定，需要通过编译器的检查，不需要借助任何自定义字符串和手动映射即可实现RPC函数绑定和调用。因此对于函数名拼写错误、函数参数数量或类型错误等情况可以直接报错，避免运行时异常。同时得益于模板类型推导，`[a]sync_struct_rpc_request`​的返回类型严格与原函数声明中的返回类型一致，不需要手动进行类型转换。
  * ​`StructRPC`​隐藏了内部实现细节，对外暴露的接口十分简洁，只需一行代码即可实现函数注册和RPC调用。
  * ​`StructRPC`支持几乎一切类型的C++函数和参数传递/结果返回形式，可以在开发中做到按照单进程的方式进行编码，并只需少量修改即可按照多进程分布式的方式执行。
* ​`StructRPC`​封装的TCPServer采用最新的Asio with C++20 coroutine实现，避免任何操作阻塞工作线程，少量线程即可支持高并发连接和高请求QPS。同时远程调用函数自身也支持使用协程实现，在链式RPC调用、请求数据库等场景下都可以有更好的性能表现。
* TCP编码协议使用自研的`StructBuffer`​，经过性能测试在多数常见场景下均有较高的序列化和反序列化速度。

‍

## Benchmark

参考StructRPC/trunk/benchmark/ 

编译器gcc13.2， -O2     server线程数固定为4，构造客户端建立固定数目的并发连接，每条连接循环进行RPC请求。

测试在固定使用echo函数下不同并发和请求字符串长度下的最大处理QPS和平均响应延迟如下：

|并发连接数|请求字符串长度|最大处理QPS|平均响应时间|
| ------------| ----------------| -------------| --------------|
|100|100|23895|3.77ms|
|100|10000|21527|4.01ms|
|1000|100|20109|48.99ms|
|1000|10000|19776|49.72ms|

‍
## 原理解析
[StructRPC原理](./doc/doc.md)