#pragma once
#include "../StructBuffer/trunk/macros.h"
#include "../StructBuffer/trunk/serializer.hpp"
#include "../StructBuffer/trunk/deserializer.hpp"

template <typename... Args>
struct CommonParamStruct
{
    DEF_DATA_STUCT;
    DEFINE_GET_FUNC_1(data);

    std::tuple<Args...> data;
};

struct TCPRequest
{
    DEF_DATA_STUCT;
    DEFINE_GET_FUNC_3(class_name, method, data);

    std::string class_name;
    std::string method;
    std::string data;
};

struct TCPResponse
{
    DEF_DATA_STUCT;
    DEFINE_GET_FUNC_2(retcode, data);

    int32_t retcode;
    std::string data;
};