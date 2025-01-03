## 背景知识

* [boost.asio C++20 Coroutines Support](https://think-async.com/Asio/boost_asio_1_27_0/doc/html/boost_asio/overview/composition/cpp20_coroutines.html)
* 可变参数模板、折叠表达式、std::apply、std::bind_front： 见cppreference相关条目
* [function_traits- 函数特征提取](https://medium.com/@matthieu.dorier/deducing-argument-and-return-types-of-a-callable-in-c-834598f6a385)
* [自研序列化和反序列化协议：StructBuffer](https://github.com/sciencnology/StructBuffer)


## 函数特征萃取

参考`struct_rpc::trait_helper::function_traits` ，利用模板偏特化实现了一系列函数特征萃取，包括获取指定函数的返回类型、参数类型列表、成员函数对应的类类型（如有）、是否为成员函数、是否为asio协程等。


## 函数绑定

#### 请求Path生成

在StructRPC中，RPC函数的注册和调用格式均为使用函数指针作为对应模板函数的非类型参数进行代码生成，比如`server.RegisterServerFunctions<&add>();`。对应的函数内部对函数指针产生一个唯一的编译期字符串作为请求path，对应的代码参考`struct_rpc::trait_helper::struct_rpc_func_path()`函数。

对于某个函数签名`int32_t MyClass::test_func(int32_t a, int32_t b);`，`struct_rpc_func_path`生成的path字符串为`MyClass::test_func--int (*)(int, int)`，其中`--`分隔符前一部分为函数名（包含附属类名称、命名空间名称等信息），后一部分为函数类型（包含参数和返回值类型等信息）。因此，只要两个函数的函数名和类型的组合不同既可在StructRPC中注册成不同的两个远程调用路径。

为了实现以上功能，框架实现了`get_func_name()`和`get_type_name()`两个constexpr函数，分别在编译期获得函数的函数名和函数类型，最后将二者返回结果拼接即可。上述两个函数的实现思路类似，这里取后者进行解析：

#### 从函数指针到编译期字符串

常见编译器都提供了获取当前函数名称的宏定义（比如gcc和clang的[`\_\_PRETTY\_FUNCTION\_\_`](https://gcc.gnu.org/onlinedocs/gcc/Function-Names.html)）。该宏会返回函数签名，并且对于模板函数会附带具体的模板参数信息，例如：

```c++
#include <iostream>

namespace test_ns {
    struct test_struct {
        void test_func() {
            std::cout << __PRETTY_FUNCTION__ << std::endl;
        }

        template <typename T>
        void test_func1() {
            std::cout << __PRETTY_FUNCTION__ << std::endl;
        }
    };
}

int main() {
    test_ns::test_struct test;
    test.test_func();   // void test_ns::test_struct::test_func()
    test.test_func1<double>();  // void test_ns::test_struct::test_func1() [with T = double]
}
```

可以看到，__PRETTY_FUNCTION__宏不仅会包括函数的详细信息（包括成员函数的类型名称和命名空间名称），对于模板函数还会包含模板参数信息。基于这个事实，只要定义一个接收模板参数T的模板函数，并使用某个RPC函数的类型实例化该模板函数，__PRETTY_FUNCTION__宏就可以得到代表该RPC函数的类型的字符串，比如：

```c++
#include <iostream>

namespace test_ns {
    struct test_struct {
        double test_rpc_func(int a) {
        }
    };
}

template <typename T>
void type_name_getter() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
}

int main() {
    type_name_getter<decltype(&test_ns::test_struct::test_rpc_func)>(); // void type_name_getter() [with T = double (test_ns::test_struct::*)(int)]
}
```

在type_name_getter中成功得到了模板参数`decltype(&test_ns::test_struct::test_rpc_func)`的类型信息（`with T = double (test_ns::test_struct::*)(int)`）。

在以上工作的基础上，只需要对__PRETTY_FUNCTION__的返回结果进行裁剪，只保留`[with T` 和`]`之间的部分即可拿到我们需要的类型信息。得益于C++标准库中`std::string_view::find`和`std::string_view::substr`函数已被标记为constexpr，只要将`__PRETTY_FUNCTION__`转为string_view，并进行适当的查找和剪切操作即可得到constexpr的`get_func_name()`函数，详细实现可以参考源码。

#### RPC函数归一化注册

用户自定义的RPC函数其类型（参数列表、返回值类型、是否为成员函数等）存在无数种可能，而RPC框架需要将其统一存储到某个容器（比如std::map）中方便根据请求path进行查找。由于C++类型系统的限制，容器中的元素类型必须完全一致，`StructRPC`采用了类型擦除、函数指针模板参数、Partial Application等技术实现上述归一化注册。

1. **统一函数签名**

由于不同的RPC函数在参数类型、数量、返回值类型上可能不同，需要设计一个统一的函数签名屏蔽上述区别。考虑到RPC TCP协议包的设计，这里采用的统一函数签名为`std::string(std::string_view)`。函数输入参数为客户端发送请求中全部函数参数序列化成的二进制字符串，在处理函数内部将其反序列化成std::tuple后输入给对应的RPC函数进行调用，将任意类型的调用结果序列化成二进制字符串后返回。

2. **函数指针模板参数**

上一步中介绍需要将输入的string_view解析成std::tuple<Args...>，其中具体函数参数Args...类型的获取需要依赖原始函数的信息通过function_traits进行提取。这里[将原始函数作为模板非类型参数](https://stackoverflow.com/a/67216795)，将上述函数签名改进为`template<auto FuncPtr> std::string(std::string_view)`即可。

3. **std::fuction类型擦除**

由于C++模板实例化原理，不同的模板参数实例化的模板函数其类型也不同，即使其函数签名完全相同。使用函数指针或者std::function再次封装都可以抹除上述差别，返回类型完全一致的可调用对象并可以存储到std::map中。简单起见项目采用了std::function,但是需要注意的是std::function并非零开销抽象，其内部采用了虚函数+可能的动态内存申请。

4. **Partial Application**

第一步介绍了解析输入参数的tuple并传递给注册的原始函数调用，该方式在注册成员函数时会出现问题：类的非静态成员函数存在一个隐式参数为类实例的指针，直接传入tuple会导致参数数量不匹配，调用失败。本框架采用函数式编程中Partial Application的思想，在注册函数时对于成员函数额外获取一份类的对象实例，将其绑定到原始的成员函数的第一个参数，并返回一个接收剩余参数的新函数。[std::bind_front](https://en.cppreference.com/w/cpp/utility/functional/bind_front)可以针对任意数量函数参数执行上述绑定过程。


## RPC客户端

#### RPC TCP请求流程

参考`struct_rpc::TCPConnectionBase::sync_struct_rpc_request`。该函数签名为`template <auto Func, typename... Args> auto sync_struct_rpc_request(Args&&... args)`，其中Func为远程调用函数的指针，Args...为远程调用的参数列表。

该函数内部进行如下操作：

1. 提取出Func的参数类型列表，定义一个元素类型与之相同的std::tuple，并通过完美转发使用传入的args...构造该tuple。此时如果传入参数的数量或者类型不正确会编译失败。
2. 使用`struct_rpc_func_path`获取Func对应的RPC请求路径，同时使用StructBuffer将第一步得到的tuple序列化为std::string，将二者组合成TCPRequest对象
3. 将第二步得到的request对象序列化成std::string并通过TCP传输给server，等待server的响应
4. 提取出Func的返回类型，使用StructBuffer从server响应信息中解析函数返回结果。如果函数存在引用类型的参数，则从响应信息中提取出server返回的函数参数并赋值给输入参数，随后返回给调用者。

得益于模板，`sync_struct_rpc_request` 实际上会在编译期对每个远程调用函数生成一份独有的实例，其参数和返回值类型与远程调用函数完全匹配。


## RPC服务端

#### RPC TCP响应流程

参考`struct_rpc::TCPServer::handle_client`。该协程维护与客户端的TCP连接，并在指定时间内未收到请求则自动销毁。

该函数操作步骤如下：

1. 读取TCP请求的头部，解析出该次请求总长度信息后读取整个TCP请求包，并使用StructBuffer将其解析成TCPRequest结构体。
2. 根据结构体中的path字段查找对应的RPC函数，利用`function_traits`提取出函数的参数和返回值特征，并根据参数类型列表反序列化TCPRequest结构体中打包的参数字段。
3. 利用std::apply将参数tuple解包并传递给预先注册的RPC处理函数调用，将返回值和调用后的函数参数序列化成TCPRespose结构体，并使用StructBuffer序列化成二进制流后写回socket。

#### TCPServer模型

`StructRPC`的TCPServer依靠Boost.Asio和C++20 coroutine特性实现了一个高效的异步RPC服务器，其基本思想如下：

* 初始化时分配固定数量的工作线程，运行同一个io_context的事件循环，每个工作线程地位均等。
* 启动单个协程循环异步接收来自客户端的TCP连接。
* 对每一个TCP连接建立一个新的协程循环处理该连接上的TCP请求。
* 对于注册的普通RPC函数（非协程），工作线程会同步执行该函数直到函数返回，期间不会中断而调度到其他协程异步操作中。对于注册的异步RPC协程（返回类型为boost::asio::awaitable<T>的协程），工作线程在执行到内部的异步操作时可能出现协程切换，并且需要注意在同一个协程暂停点前后可能被不同的工作线程执行，因此框架要求继承`ThreadLocalSingleton`（每线程一份实例的单例类）不允许注册RPC协程。
* 单条连接的处理协程任意IO操作均设置了超时时间，超时未响应则直接退出该协程并清理资源，实现自动伸缩的并发连接池。
* 由于全部阻塞操作均采用协程实现，使用少量线程即可支持高并发连接和高请求QPS，且实现十分简洁。