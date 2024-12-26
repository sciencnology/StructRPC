#ifndef SO_WRAPPER_H
#define SO_WRAPPER_H

#include <utility>
#include <map>
#include <string>
#include <string_view>
#include <functional>
#include <boost/asio/awaitable.hpp>
#include <boost/asio.hpp>
#include "nlohmann/json.hpp"
#include "util.hpp"

namespace asio = boost::asio;
using json = nlohmann::json;
using ProcessFunc = std::function<std::string(json &)>;
using ProcessCoroutine = std::function<asio::awaitable<std::string>(json &)>;
using TCPProcessCoroutine = std::function<asio::awaitable<std::string>(std::string_view)>;
using TCPProcessFunc = std::function<std::string(std::string_view)>;
using ProcessFuncMap = std::map<std::string, std::map<std::string, ProcessFunc>>;
using ProcessCoroutineMap = std::map<std::string, std::map<std::string, ProcessCoroutine>>;
using TCPProcessCoroutineMap = std::map<std::string_view, TCPProcessCoroutine>;
using TCPProcessFuncMap = std::map<std::string_view, TCPProcessFunc>;

#endif