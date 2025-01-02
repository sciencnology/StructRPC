#include "functions.hpp"
#include <format>
#include <memory>
#include "../utils/timer.hpp"


int main(int argc, char* argv[])
{
    uint32_t thread_num = std::stoi(argv[1]);
    uint32_t concurrency = std::stoi(argv[2]);
    uint32_t seconds = std::stoi(argv[3]);
    const char* port = argv[4];

    BenchmarkRecorder recorder;
    io_context ioc;
    boost::asio::thread_pool thread_pool(thread_num);

    std::atomic<bool> need_stop = false;
    for (uint32_t i = 0; i < concurrency; ++i) {
        co_spawn(ioc, [&]() -> awaitable<void> {
            std::unique_ptr<TCPConnectionBase> async_connection_ptr = std::make_unique<AsyncTCPConnection>("127.0.0.1", port, ioc);
            while (!need_stop.load(std::memory_order_acquire)) {
                TimerRaii timer([&](double milliseconds)
                            { recorder.add(milliseconds); });
                co_await async_connection_ptr->async_struct_rpc_request<&rpc_benchmark::echo>("testbenchmarkstring");
            }
        }, detached);
    }
    
    for (int i = 0; i < thread_num; ++i)
    {
        boost::asio::post(thread_pool, [&]
                            { ioc.run(); });
    }

    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    need_stop.store(true, std::memory_order_release);

    LOG("total requested {}, avg timecost {}", recorder.total_requests.load(), (recorder.total_timecost.load() + 0.0) / recorder.total_requests.load());
    return 0;
}