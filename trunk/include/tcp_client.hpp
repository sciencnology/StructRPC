#pragma once
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <unordered_map>
#include <list>
#include <thread>
#include <mutex>
#include <iostream>
#include <chrono>
#include <string>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class TcpConnection {
public:
    TcpConnection(asio::io_context& io_context, const std::string& address, uint16_t port)
        : socket_(io_context), address_(address), port_(port) {}

    asio::awaitable<void> connect() {
        tcp::resolver resolver(socket_.get_executor());
        auto endpoints = co_await resolver.async_resolve(address_, std::to_string(port_), asio::use_awaitable);
        co_await asio::async_connect(socket_, endpoints, asio::use_awaitable);
    }

    asio::awaitable<std::string> send_and_receive(const std::string& request) {
        co_await asio::async_write(socket_, asio::buffer(request), asio::use_awaitable);

        std::string response(1024, '\0');
        auto n = co_await asio::async_read(socket_, asio::buffer(response), asio::use_awaitable);
        response.resize(n);

        co_return response;
    }

    bool is_open() const {
        return socket_.is_open();
    }

    void close() {
        asio::error_code ec;
        socket_.close(ec);
    }

private:
    tcp::socket socket_;
    std::string address_;
    uint16_t port_;
};

class TcpConnectionPool {
public:
    TcpConnectionPool(size_t max_connections)
        : max_connections_(max_connections) {}

    asio::awaitable<std::string> request(const std::string& address, uint16_t port, const std::string& message) {
        auto& thread_local_connections = connections_.get();
        auto& thread_local_lru_list = lru_list_.get();

        auto key = address + ":" + std::to_string(port);
        auto it = thread_local_connections.find(key);

        if (it == thread_local_connections.end()) {
            if (thread_local_connections.size() >= max_connections_) {
                evict_connection(thread_local_connections, thread_local_lru_list);
            }

            auto connection = std::make_shared<TcpConnection>(io_context_, address, port);
            co_await connection->connect();
            thread_local_lru_list.emplace_front(key, connection);
            thread_local_connections[key] = thread_local_lru_list.begin();
        } else {
            thread_local_lru_list.splice(thread_local_lru_list.begin(), thread_local_lru_list, it->second);
        }

        auto& connection = *thread_local_connections[key]->second;
        auto response = co_await connection.send_and_receive(message);
        co_return response;
    }

private:
    void evict_connection(
        std::unordered_map<std::string, decltype(lru_list_.get().begin())>& local_connections,
        std::list<std::pair<std::string, std::shared_ptr<TcpConnection>>>& local_lru_list) {
        if (local_lru_list.empty()) {
            return;
        }

        auto& key = local_lru_list.back().first;
        auto& connection = local_lru_list.back().second;

        connection->close();
        local_connections.erase(key);
        local_lru_list.pop_back();
    }

    // 使用 thread_local 实现线程独立的连接池
    struct ThreadLocalData {
        std::list<std::pair<std::string, std::shared_ptr<TcpConnection>>> lru_list;
        std::unordered_map<std::string, decltype(lru_list_.get().begin())> connections;
    };

    // 每线程独立的连接池数据
    thread_local static inline ThreadLocalData thread_local_data_;

    size_t max_connections_;
    asio::io_context io_context_;
};

// int main() {
//     try {
//         TcpConnectionPool pool(4, 10); // 4 threads in the pool, max 10 connections

//         asio::co_spawn(pool.get_io_context(), [&]() -> asio::awaitable<void> {
//             try {
//                 auto response = co_await pool.request("example.com", 80, "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n");
//                 std::cout << "Response: " << response << std::endl;
//             } catch (const std::exception& e) {
//                 std::cerr << "Error: " << e.what() << std::endl;
//             }
//         }, asio::detached);

//         std::this_thread::sleep_for(std::chrono::seconds(10));
//     } catch (const std::exception& e) {
//         std::cerr << "Exception: " << e.what() << std::endl;
//     }

//     return 0;
// }
