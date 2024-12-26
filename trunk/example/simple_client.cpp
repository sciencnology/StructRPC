#include "functions.hpp"
#include <format>

using std::cout;
using std::endl;
using std::format;

template <size_t N>
void print_array(const std::array<char, N> input) {
    for (size_t i = 0; i < N; ++i) {
        cout << input[i];
    }
    cout << endl;
}

int main()
{
    // 可以利用C++编译期静态类型检查机制，函数传参数量或类型出错时在编译期即可暴露问题
    SyncTCPConnection sync_connection("127.0.0.1", "8080");
    cout << sync_connection.sync_struct_rpc_request<&TCPTestServer::add>(1, 2) << endl;
    cout << sync_connection.sync_struct_rpc_request<&free_add>(1, 2) << endl;
    cout << sync_connection.sync_struct_rpc_request<&TCPTestServer::static_sub>(1, 2) << endl;

    cout << sync_connection.sync_struct_rpc_request<&TCPTestServer::generic_add_various_params<int, int, double>>(1, 2, 3.5) << endl;
    cout << sync_connection.sync_struct_rpc_request<&TCPTestServer::generic_add_various_params<int, int>>(10, 10) << endl;


    // print_array(trait_helper::func_name<&TCPTestServer::add>());
    // print_array(trait_helper::get_type_name<decltype(&TCPTestServer::add)>());

    // // print_array(trait_helper::concatenate(trait_helper::func_name<&TCPTestServer::add>(), trait_helper::get_type_name<decltype(&TCPTestServer::add)>()));

    // std::cout << trait_helper::struct_rpc_func_path<&TCPTestServer::add>() << endl;
    return 0;
}