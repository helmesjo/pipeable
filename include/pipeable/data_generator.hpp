#pragma once

#include <pipeable/pipeable.hpp>
#include <functional>
#include <type_traits>

namespace pipeable
{
    namespace impl
    {
        template<typename... outputs_t>
        struct data_generator_impl : impl::custom_pipeable_tag
        {
            void operator()(const outputs_t&... args)
            {
                downstream_(args...);
            }

            template<typename callable_t,
                concepts::IsInvocable<callable_t, outputs_t...> = nullptr>
                data_generator_impl& operator>>=(callable_t&& downstream)
            {
                downstream_ = [downstream = std::forward<callable_t>(downstream)](auto&&... args) mutable
                {
                    invocation::invoke(std::forward<callable_t>(downstream), std::forward<decltype(args)>(args)...);
                };

                return *this;
            }

        private:
            std::function<void(const outputs_t&...)> downstream_ = [](const auto&...) {};
        };

        template<typename... bases_t>
        struct multi_generator_impl : bases_t...
        {
            using bases_t::operator()...;
            using bases_t::operator>>=...;
        };
    }

    template<typename... outputs_t>
    struct data_generator : impl::multi_generator_impl<impl::data_generator_impl<outputs_t>...>
    {
    };
}