#pragma once

#include <string>
#include <vector>

namespace struct_rpc
{

namespace util
{

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
