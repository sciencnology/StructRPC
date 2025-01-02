#include "functions.hpp"
#include <format>
#include <memory>
using std::cout;
using std::endl;
using std::format;

awaitable<void> rpc_coro_1(std::unique_ptr<TCPConnectionBase> async_connection_ptr)
{
    // 依次顺序执行下面4次rpc请求，其中第三次会阻塞3s后返回
    auto [_1,_2]  = std::tuple{
        co_await async_connection_ptr->async_struct_rpc_request<add>(1, 2),
        co_await async_connection_ptr->async_struct_rpc_request<echo>("helloworld")
    };
    cout << format("{}\n{}\n", _1,_2); 

    auto [_3,_4]  = std::tuple{
        co_await async_connection_ptr->async_struct_rpc_request<wait3s_and_echo>(1),
        co_await async_connection_ptr->async_struct_rpc_request<generic_add_various_params<int, int, double>>(1, 2, 3.5)
    };  
    cout << format("{}\n{}\n", _3,_4); 
}

awaitable<void> rpc_coro_2(std::unique_ptr<TCPConnectionBase> async_connection_ptr)
{
    auto coro_ret = co_await async_connection_ptr->async_struct_rpc_request<generic_add<int>>(1, 2);
    cout << coro_ret << endl;
}

int main()
{
    io_context ioc;
    co_spawn(ioc, rpc_coro_1(std::move(std::make_unique<AsyncTCPConnection>("127.0.0.1", "8080", ioc))), detached);
    co_spawn(ioc, rpc_coro_2(std::move(std::make_unique<AsyncTCPConnection>("127.0.0.1", "8080", ioc))), detached);
    
    ioc.run();
}