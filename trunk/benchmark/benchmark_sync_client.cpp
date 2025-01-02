#include "functions.hpp"
#include <format>
#include <memory>
#include <atomic>
#include <thread>
#include "../utils/timer.hpp"

/**
 * 1. 回显函数，高请求qps
 * 2. 回显函数，高并发连接数
 * 3. 计算密集型任务的请求延迟
*/
int main(int argc, char* argv[])
{
    BenchmarkRecorder recorder;
    uint32_t thread_num = std::stoi(argv[1]);
    uint32_t seconds = std::stoi(argv[2]);
    const char* port = argv[3];
    std::vector<std::jthread> client_threads;

    for (uint32_t i = 0; i < thread_num; ++i) {
        client_threads.emplace_back([&](std::stop_token stop_token){
            std::unique_ptr<TCPConnectionBase> sync_connection_ptr = std::make_unique<SyncTCPConnection>("127.0.0.1", port);
            while (!stop_token.stop_requested()) {
                TimerRaii timer([&](double milliseconds)
                            { recorder.add(milliseconds); });
                sync_connection_ptr->sync_struct_rpc_request<&rpc_benchmark::echo>("testbenchmarkstring");
            }
        });
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    for (auto& thread : client_threads) {
        thread.request_stop();
    }
    LOG("total requested {}, avg timecost {}", recorder.total_requests.load(), (recorder.total_timecost.load() + 0.0) / recorder.total_requests.load());
    return 0;
}