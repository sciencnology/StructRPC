#pragma once

#include <string>
#include <vector>

namespace struct_rpc
{

namespace util
{
    inline std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        // 格式化时间为 yyyy-MM-dd HH:mm:ss.sss
        std::tm local_time;
    #ifdef _WIN32
        localtime_s(&local_time, &now_time_t); // Windows 平台
    #else
        localtime_r(&now_time_t, &local_time); // Linux/Unix 平台
    #endif
        char buf[64];
        ::strftime(buf, 64, "%Y-%m-%d %H:%M:%S", &local_time);
        return std::string(buf);
    }
    template <typename T>
    class Singleton
    {
    public:
        Singleton(const Singleton &) = delete;
        Singleton &operator=(const Singleton &) = delete;

        static T &getInstance()
        {
            static T instance;
            return instance;
        }

    protected:
        Singleton() = default;
        virtual ~Singleton() = default;
    };

    template <typename T>
    class ThreadLocalSingleton
    {
    public:
        ThreadLocalSingleton(const ThreadLocalSingleton &) = delete;
        ThreadLocalSingleton &operator=(const ThreadLocalSingleton &) = delete;

        static T &getInstance()
        {
            static thread_local T instance;
            return instance;
        }

    protected:
        ThreadLocalSingleton() = default;
        virtual ~ThreadLocalSingleton() = default;
    };
};
}
