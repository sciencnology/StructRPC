#include "functions.hpp"
#include <format>
#include <memory>
using std::cout;
using std::endl;
using std::format;

awaitable<void> sync_rpc_in_coro(std::unique_ptr<TCPConnectionBase> async_connection_ptr)
{
    auto [_1,_2,_3,_4]  = std::tuple{
        co_await async_connection_ptr->async_struct_rpc_request<&ExampleRPCClass::add>(1, 2),
        co_await async_connection_ptr->async_struct_rpc_request<&free_add>(1, 2),
        co_await async_connection_ptr->async_struct_rpc_request<&ExampleRPCClass::generic_add<int>>(1, 2),
        co_await async_connection_ptr->async_struct_rpc_request<&ExampleRPCClass::generic_add_various_params<int, int, double>>(1, 2, 3.5)
    };
    cout << format("{}\n{}\n{}\n{}\n", _1,_2,_3,_4);   
}

awaitable<void> async_rpc_in_coro(std::unique_ptr<TCPConnectionBase> async_connection_ptr)
{
    auto coro_ret = co_await async_connection_ptr->async_struct_rpc_request<&ExampleRPCClass::wait3s_and_echo>(1);
    cout << coro_ret << endl;
}




int main()
{
    io_context ioc;
    co_spawn(ioc, async_rpc_in_coro(std::move(std::make_unique<AsyncTCPConnection>("127.0.0.1", "8080", ioc))), detached);
    co_spawn(ioc, sync_rpc_in_coro(std::move(std::make_unique<AsyncTCPConnection>("127.0.0.1", "8080", ioc))), detached);
    
    ioc.run();

    // sleep(20);
}