#include "client_base.h"

asio::awaitable<json> ClientBase::make_request(std::string target, json &params)
{
    try
    {
        // 连接目标server
        tcp::resolver resolver(co_await asio::this_coro::executor);
        beast::tcp_stream stream(co_await asio::this_coro::executor);
        auto const results = co_await resolver.async_resolve(host, std::to_string(port), asio::use_awaitable);
        co_await stream.async_connect(results, asio::use_awaitable);

        // 写入请求
        http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.body() = params.dump();
        req.prepare_payload();
        co_await http::async_write(stream, req, asio::use_awaitable);

        // 处理和返回响应
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        co_await http::async_read(stream, buffer, res, asio::use_awaitable);
        std::cout << res << std::endl;
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        if (ec && ec != beast::errc::not_connected)
        {
            throw beast::system_error{ec};
        }
        std::string response_body = boost::beast::buffers_to_string(res.body().data());
        co_return json::parse(response_body);
    }
    catch (const std::exception &e)
    {
        co_return std::string("Error: ") + e.what();
    }
}