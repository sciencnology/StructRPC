#include "functions.hpp"
#include <format>
#include <memory>
using std::cout;
using std::endl;
using std::format;

int main()
{
    std::unique_ptr<TCPConnectionBase> sync_connection_ptr = std::make_unique<SyncTCPConnection>("127.0.0.1", "8080");
    cout << sync_connection_ptr->sync_struct_rpc_request<echo>("thisisateststring") << endl;  // returns thisisateststring
    cout << sync_connection_ptr->sync_struct_rpc_request<add>(1, 2) << endl;  // returns 3
    cout << sync_connection_ptr->sync_struct_rpc_request<generic_add<int>>(1, 2) << endl; // returns 3
    cout << sync_connection_ptr->sync_struct_rpc_request<wait3s_and_echo>(1) << endl; // after 3s, returns 1
    cout << sync_connection_ptr->sync_struct_rpc_request<generic_add_various_params<int, int, double>>(1, 2, 3.5) << endl;    // returns 6.5
    cout << sync_connection_ptr->sync_struct_rpc_request<generic_add_various_params<int, int>>(10, 10) << endl;   // returns 20
    // cout << sync_connection_ptr->sync_struct_rpc_request<addo>(10, 10) << endl;    // 函数名拼写错误，可以在编译期检查并报错
    cout << sync_connection_ptr->sync_struct_rpc_request<free_add_combined>(CombinedStruct{"hello ", 1}, CombinedStruct{"world", 2}).str_member << endl;   // returns {"hello world", 3}
    cout << sync_connection_ptr->sync_struct_rpc_request<&ExampleRPCClass::add>(10, 10) << endl;    // returns 20
    return 0;
}