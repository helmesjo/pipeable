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
            FWD(downstream)(
                FWD(elem));
        }
    });

    /* VISITOR */
    inline const auto visit = assembly::make_interceptor(
        [](auto&& downstream, auto&& variant)
    {
        std::visit(FWD(downstream), FWD(variant));
    });

    /* UNPACK */
    inline const auto unpack = assembly::make_interceptor(
        [](auto&& downstream, auto&& tuple)
    {
        std::apply(FWD(downstream), FWD(tuple));
    });

    /* MAYBE */
    inline const auto maybe = assembly::make_interceptor(
        [](auto&& downstream, auto&& optional)
    {
        if (optional)
        {
            FWD(downstream)(*optional);
        }
    });

    /*
    Chain callables. Result from left-hand callable gets passed as input to right-hand callable.
    Invoke by piping valid invocable input to left-most callable.
    */
    template<typename lhs_t, typename rhs_t,
        concepts::IsNotCustomPipeable<lhs_t> = nullptr>
    constexpr decltype(auto) operator>>=(lhs_t&& lhs, rhs_t&& rhs)
    {
        if constexpr (meta::is_invocable_v<rhs_t, lhs_t>)
        {
            return invocation::invoke(FWD(rhs), FWD(lhs));
        }
        else
        {
            return assembly::compose(FWD(lhs), FWD(rhs));
        }
    }
}