#ifndef UTIL_HPP
#define UTIL_HPP
#include <string>
#include <vector>
namespace util
{
    class StringUtil
    {
    public:
        static std::vector<std::string> split(std::string_view input, char delimiter)
        {
            std::vector<std::string> result;
            size_t start = 0;
            size_t end = 0;

            // 遍历输入字符串，查找分隔符
            while ((end = input.find(delimiter, start)) != std::string::npos)
            {
                // 如果分隔符之前有内容，则将其添加到结果中
                if (start != end)
                {
                    result.emplace_back(std::string(input.substr(start, end - start)));
                }
                start = end + 1;
            }

            // 添加最后一个子字符串
            if (start < input.length())
            {
                result.emplace_back(std::string(input.substr(start)));
            }

            return result;
        }
    };

    template <typename T>
    class Singleton
    {
    public:
        // 禁止拷贝和赋值操作
        Singleton(const Singleton &) = delete;
        Singleton &operator=(const Singleton &) = delete;

        // 获取单例实例的接口
        static T &getInstance()
        {
            static T instance;
            return instance;
        }

    protected:
        Singleton() = default;
        virtual ~Singleton() = default;
    };
};

#endif // UTIL_HPP