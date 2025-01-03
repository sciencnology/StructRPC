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