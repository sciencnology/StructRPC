#include "client_base.h"

/**
 * auto generate
 */
class TestClient : public ClientBase
{
public:
    asio::awaitable<int> testRet()
    {
        json params;
        json ret = co_await make_request("/Test/testRet", params);
        co_return ret;
    }
};

class Test2Client : public ClientBase
{
public:
    asio::awaitable<std::string> appendString(const std::string &a)
    {
        json params;
        params["a"] = a;
        json ret = co_await make_request("/Test2/appendString", params);
        co_return ret["Ret"];
    }
};