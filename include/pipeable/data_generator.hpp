#pragma once

#include <pipeable/pipeable.hpp>
#include <functional>

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
        template<typename callable_t,
            concepts::IsInvocableWithAny<callable_t, outputs_t...> = nullptr>
        data_generator& operator>>=(callable_t&& downstream)
        {
            // Recursively call >>= for each output type to find exact base/data_generator
            operator>>=<callable_t, outputs_t...>(std::forward<callable_t>(downstream));
            return *this;
        }

    private:
        template<typename callable_t, typename output_t>
        void operator>>=(callable_t&& downstream)
        {
            // Register callable for matching output only
            if constexpr (meta::is_invocable_v<callable_t, output_t>)
            {
                // Explicitly call correct base to avoid ambiguity
                using exact_base_t = impl::data_generator_impl<output_t>;
                static_cast<exact_base_t*>(this)->operator>>=(std::forward<callable_t>(downstream));
            }
        }

        template<typename callable_t, typename output_t, typename tail_t, typename... tails_t>
        void operator>>=(callable_t&& downstream)
        {
            operator>>=<callable_t, output_t>(std::forward<callable_t>(downstream));
            // Recursively register callable for each (matching) output type
            operator>>=<callable_t, tail_t, tails_t...>(std::forward<callable_t>(downstream));
        }
    };
}