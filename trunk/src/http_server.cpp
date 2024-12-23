#include "http_server.hpp"

HttpServer::HttpServer(int thread_cnt, int port) : thread_cnt_(thread_cnt), port_(port)
{
    init_process_map(proccess_func_map_, proccess_coroutine_map_);
}

void HttpServer::start()
{
    for (int i = 0; i < thread_cnt_; ++i)
    {
        auto io_context = std::make_shared<asio::io_context>();
        io_contexts_.push_back(io_context);
        work_guards_.emplace_back(asio::make_work_guard(*io_context));
    }

    tcp::endpoint endpoint(tcp::v4(), port_);

    // 启动 I/O 线程
    std::vector<std::thread> threads_vec;
    for (int i = 0; i < thread_cnt_; ++i)
    {
        threads_vec.emplace_back([&, i]
                                 {
                asio::co_spawn(*io_contexts_[i], do_listen(*io_contexts_[i], endpoint), asio::detached);
                io_contexts_[i]->run(); });
    }

    // 等待 I/O 线程结束
    for (auto &t : threads_vec)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
}

void HttpServer::stop()
{
    running_.store(false, std::memory_order_relaxed);
}

asio::awaitable<void> HttpServer::do_session(tcp::socket socket)
{
    try
    {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        co_await http::async_read(socket, buffer, req, asio::use_awaitable);

        // 将请求放入队列
        auto request = std::make_shared<Request>(std::move(socket), std::move(req));
        // std::cout << "enqueue succ " << request_queue_.size_approx() << std::this_thread::get_id() << std::endl;
        // asio::co_spawn(co_await asio::this_coro::executor, process_single_request(request), asio::detached);
        co_await process_single_request(request);
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

asio::awaitable<void> HttpServer::do_listen(asio::io_context &ioc, tcp::endpoint endpoint)
{
    tcp::acceptor acceptor(ioc, endpoint);
    while (running_.load(std::memory_order_relaxed))
    {
        tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
        asio::co_spawn(ioc, do_session(std::move(socket)), asio::detached);
    }
}

asio::awaitable<void> HttpServer::process_single_request(std::shared_ptr<Request> request)
{
    auto &req = request->req;
    tcp::socket &socket = request->socket;
    try
    {
        if (req.method() != http::verb::post)
        {
            std::cout << "Received unknown method: " << req.method() << std::endl;
            // TODO： use defer
            do_response<http::status::method_not_allowed>(socket.get_executor(), request, "Method Not Allowed", req.version());
            co_return;
        }

        std::vector<std::string> target_vec = util::StringUtil::split(std::string(req.target()), '/');
        // 请求uri: http://ip:port/ClassName/MethodName
        ProcessCoroutine func;
        try
        {
            func = proccess_coroutine_map_.at(std::string(target_vec.at(0))).at(std::string(target_vec.at(1)));
        }
        catch (const std::out_of_range &e)
        {
            do_response<http::status::not_found>(socket.get_executor(), request, "Not Found", req.version());
            co_return;
        }

        try
        {
            json req_json = json::parse(req.body());
            std::string rsp_body = co_await func(req_json);
            std::cout << "rsp_body= " << rsp_body << std::endl;
            do_response<http::status::ok>(socket.get_executor(), request, rsp_body, req.version());
            std::cout << "do_rsp succ" << rsp_body << std::endl;
        }
        catch (json::parse_error &e)
        {
            do_response<http::status::bad_request>(socket.get_executor(), request, "Invalid JSON", req.version());
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        do_response<http::status::internal_server_error>(socket.get_executor(), request, std::string("Internal Server Error: ") + e.what(), req.version());
    }
}

template <http::status RSP_CODE>
void HttpServer::do_response(boost::asio::any_io_executor executor, std::shared_ptr<Request> request, std::string_view body, unsigned int version)
{
    http::response<http::string_body> res{RSP_CODE, version};
    res.set(http::field::server, "Beast");
    res.set(http::field::content_type, "text/plain");
    res.body() = body;
    res.prepare_payload();
    asio::co_spawn(executor, [request, res]() -> asio::awaitable<void>
                   { co_await http::async_write(request->socket, res, asio::use_awaitable); }, asio::detached);
}