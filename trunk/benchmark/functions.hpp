#pragma once
#include <string>
#include <atomic>
#include "../struct_rpc.hpp"

using namespace struct_rpc;

namespace rpc_benchmark
{
    // 基础回显函数
    inline std::string echo(std::string input) {
        return input;
    }
}


struct BenchmarkRecorder
{
    std::atomic<uint32_t> total_requests;
    std::atomic<uint32_t> total_timecost;
    void add(uint32_t timecost_ms) {
        total_requests.fetch_add(1, std::memory_order_relaxed);
        total_timecost.fetch_add(timecost_ms, std::memory_order_relaxed);
    }
};
