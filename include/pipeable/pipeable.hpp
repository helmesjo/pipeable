#pragma once

#include <pipeable/internal/pipeable_internal.hpp>
#include <pipeable/internal/type_traits.hpp>
#include <variant>

namespace pipeable
{
    /* FOR EACH */
    inline const auto for_each = assembly::make_interceptor(
        [](auto&& downstream, auto&& iterable)
    {
        static_assert(pipeable::type::is_iterable_v<decltype(iterable)>, "for_each requires iterable input.");
        // Iterate with universal reference, and perfectly forward to downstream pipeline
        for(auto&& elem : iterable)
        {
            std::forward<decltype(downstream)>(downstream)(
                std::forward<decltype(elem)>(elem));
        }
    });

    /* VISITOR */
    inline const auto visit = assembly::make_interceptor(
        [](auto&& downstream, auto&& variant)
    {
        std::visit(std::forward<decltype(downstream)>(downstream), std::forward<decltype(variant)>(variant));
    });

    /* UNPACK */
    inline const auto unpack = assembly::make_interceptor(
        [](auto&& downstream, auto&& tuple)
    {
        std::apply(std::forward<decltype(downstream)>(downstream), std::forward<decltype(tuple)>(tuple));
    });

    /* MAYBE */
    inline const auto maybe = assembly::make_interceptor(
        [](auto&& downstream, auto&& optional)
    {
        if (optional)
        {
            std::forward<decltype(downstream)>(downstream)(*optional);
        }
    });

    /*
    Chain callables. Result from left-hand callable gets passed as input to right-hand callable.
    Invoke by piping valid invocable input to left-most callable.
    */
    template<typename lhs_t, typename rhs_t>
    constexpr decltype(auto) operator>>=(lhs_t&& lhs, rhs_t&& rhs)
    {
        if constexpr (meta::is_invocable_v<rhs_t, lhs_t>)
        {
            return invocation::invoke(std::forward<rhs_t>(rhs), std::forward<lhs_t>(lhs));
        }
        else
        {
            return assembly::compose(std::forward<lhs_t>(lhs), std::forward<rhs_t>(rhs));
        }
    }
}