#pragma once

#include <functional>
#include <tuple>

#include <boost/asio.hpp>




namespace struct_rpc {
namespace net = boost::asio;
using namespace boost::asio::awaitable_operators;
using namespace std::chrono_literals;

using tcp = net::ip::tcp;
using error_code = boost::system::error_code;
using system_error = boost::system::system_error;
using time_point = typename std::chrono::steady_clock::time_point;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
using boost::asio::experimental::as_tuple;
using std::chrono::milliseconds;
using std::chrono::seconds;

enum class client_connection_state : uint8_t;
enum class server_connection_state : uint8_t;

/**
 * @brief The type that is returned from a read attempt.
 * @param error The error code returned. If there was no error it will
 * return a value of 0 or asio::error::Success.
 * @param bytes_transferred The number of bytes transferred during the read.
 */
using read_result_t = std::tuple<cpool::error, std::size_t>;

/**
 * @brief The type that is returned from a write attempt.
 * @param error The error code returned. If there was no error it will
 * return a value of 0 or asio::error::Success.
 * @param bytes_transferred The number of bytes transferred during the read.
 */
using write_result_t = std::tuple<cpool::error, std::size_t>;

/**
 * @brief The function object that is called whenever the status of the
 * TcpAcceptor object has changed state.
 * @param state The new state of the object as defined by the enum
 * sio::network::ConnectionState.
 */
template <class T>
using connection_state_change_handler = std::function<awaitable<error>(
    T* conn, const client_connection_state state)>;

namespace detail {
/**
 * @brief The type that is returned from a read attempt.
 * @param error The error code returned. If there was no error it will
 * return a value of 0 or asio::error::Success.
 * @param bytes_transferred The number of bytes transferred during the read.
 */
using asio_read_result_t = std::tuple<boost::system::error_code, std::size_t>;

/**
 * @brief The type that is returned from a write attempt.
 * @param error The error code returned. If there was no error it will
 * return a value of 0 or asio::error::Success.
 * @param bytes_transferred The number of bytes transferred during the read.
 */
using asio_write_result_t = std::tuple<boost::system::error_code, std::size_t>;
} // namespace detail

} // namespace cpool