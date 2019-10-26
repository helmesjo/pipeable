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
        static_assert(fho::type::is_iterable_v<decltype(iterable)>, "for_each requires iterable input.");
        // Iterate with universal reference, and perfectly forward to downstream pipeline
        for(auto&& elem : iterable)
        {
            std::forward<decltype(downstream)>(downstream)(
                std::forward<decltype(elem)>(elem));
        }
    });

    /* VISITOR */
    inline const auto visitor = assembly::make_interceptor(
        [](auto&& downstream, auto&& variant)
    {
        std::visit(std::forward<decltype(downstream)>(downstream), std::forward<decltype(variant)>(variant));
    });

    /* MAYBE */
    inline const auto maybe = assembly::make_interceptor(
        [](auto&& downstream, auto&& optional)
    {
        if (optional.has_value())
        {
            std::forward<decltype(downstream)>(downstream)(*optional);
        }
    });

    template<typename lhs_t, typename rhs_t>
    constexpr decltype(auto) operator>>=(lhs_t&& lhs, rhs_t&& rhs)
    {
        if constexpr (meta::is_invocable_v<decltype(rhs), decltype(lhs)>)
        {
            return invocation::invoke(std::forward<rhs_t>(rhs), std::forward<lhs_t>(lhs));
        }
        else
        {
            return assembly::compose(std::forward<lhs_t>(lhs), std::forward<rhs_t>(rhs));
        }
    }
}