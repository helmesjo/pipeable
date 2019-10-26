#pragma once

#include <type_traits>

namespace fho::type
{
    namespace details
    {
        template <typename T, typename = void>
        struct is_iterable : std::false_type {};
        template <typename T>
        struct is_iterable<T, std::void_t<decltype(std::declval<T>().begin()),
            decltype(std::declval<T>().end())>>
            : std::true_type {};
    }
    template <class T>
    constexpr bool is_iterable_v = details::is_iterable<T>::value;
}