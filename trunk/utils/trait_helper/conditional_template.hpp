#pragma once

#include <type_traits>

namespace struct_rpc
{
namespace trait_helper
{
    /**
     * @brief: 条件函数模板，当Cond为true时返回Template<T>，否则返回T
    */
    template <bool Cond, template <typename> class Template, typename T>
    using conditional_template_t = std::conditional_t<Cond, Template<T>, T>;
}
}

