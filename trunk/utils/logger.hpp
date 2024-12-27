#pragma once
#include <iostream>
#include <string>
#include <format>
#include <chrono>
#include <ctime>
#include "util.hpp"

namespace struct_rpc
{
namespace util
{
/**
 * @brief: 将字符串字面量转为可用于非类型模板参数的编译期常量
*/
template<size_t N>
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }
    
    char value[N];
};

/**
 * @brief: 打印日志。由于std::format要求传入的格式化字符串为编译期常量，故放在模板参数中
*/
template <StringLiteral FMT, typename... Args>
inline void logMessage(std::string_view file, const char* function, int line, Args&&... args) {
    std::string user_message = std::format(FMT.value, std::forward<Args>(args)...);
    std::string log_entry = std::format("[{}] [{}:{}:{}] {}",
                                        getCurrentTime(), file, function, line, user_message);
    std::cout << log_entry << std::endl;
}

/**
 * @brief: 裁剪__FILE__宏返回的文件名（默认会返回全路径，裁剪后只返回文件名）
*/
inline constexpr std::string_view extractFileName(const std::string_view path) {
#ifdef _WIN32
    constexpr char separator = '\\';
#else
    constexpr char separator = '/';
#endif
    size_t pos = path.rfind(separator);
    return (pos == std::string_view::npos) ? path : path.substr(pos + 1);
}


#define LOG(fmt, ...) struct_rpc::util::logMessage<fmt>(struct_rpc::util::extractFileName(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)

}
}