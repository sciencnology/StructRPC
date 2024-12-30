#include "functions.hpp"
#include <format>
#include <memory>
using std::cout;
using std::endl;
using std::format;

int main()
{
    // 可以利用C++编译期静态类型检查机制，函数传参数量或类型出错时在编译期即可暴露问题
    std::unique_ptr<TCPConnectionBase> sync_connection_ptr = std::make_unique<SyncTCPConnection>("127.0.0.1", "8080");
    // SyncTCPConnection sync_connection("127.0.0.1", "8080");
    cout << sync_connection_ptr->sync_struct_rpc_request<&ExampleRPCClass::add>(1, 2) << endl;
    cout << sync_connection_ptr->sync_struct_rpc_request<&free_add>(1, 2) << endl;
    cout << sync_connection_ptr->sync_struct_rpc_request<&ExampleRPCClass::generic_add<int>>(1, 2) << endl;
    cout << sync_connection_ptr->sync_struct_rpc_request<&ExampleRPCClass::wait3s_and_echo>(1) << endl;
    cout << sync_connection_ptr->sync_struct_rpc_request<&ExampleRPCClass::generic_add_various_params<int, int, double>>(1, 2, 3.5) << endl;
    cout << sync_connection_ptr->sync_struct_rpc_request<&ExampleRPCClass::generic_add_various_params<int, int>>(10, 10) << endl;
    // cout << sync_connection_ptr->sync_struct_rpc_request<&ExampleRPCClass::addo>(10, 10) << endl;
    sleep(10);
    cout << sync_connection_ptr->sync_struct_rpc_request<&ExampleRPCClass::generic_add_various_params<int, int>>(10, 10) << endl;
    return 0;
}