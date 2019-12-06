#pragma once

#include <pipeable/pipeable.hpp>
#include <functional>
#include <vector>

namespace pipeable
{
    namespace impl
    {
        template<typename... outputs_t>
        struct data_generator_impl : impl::custom_pipeable_tag
        {
            using downstream_t = std::function<void(const outputs_t&...)>;

            void operator()(const outputs_t&... args)
            {
                for (auto& downstream : receivers_)
                {
                    downstream.second(args...);
                }
            }

            template<typename callable_t,
                concepts::IsInvocable<callable_t, outputs_t...> = nullptr>
            void operator+=(callable_t&& downstream)
            {
                auto id = identifier(downstream);
                downstream_t receiverCall = [downstream = downstream](auto&& arg) mutable
                {
                    invocation::invoke(std::forward<callable_t>(downstream), std::forward<decltype(args)>(args)...);
                };
                receivers_.emplace_back(id, receiverCall);
            }

            template<typename callable_t,
                concepts::IsInvocable<callable_t, outputs_t...> = nullptr>
            void operator-=(callable_t&& downstream)
            {
                receivers_.erase(std::remove_if(receivers_.begin(), receivers_.end(), [addr = identifier(downstream)](auto&& receiver) {
                    return receiver.first == addr;
                }), receivers_.end());
            }

        private:
            template<typename callable_t>
            static constexpr const void* identifier(callable_t&& callable)
            {
                if constexpr (std::is_pointer_v<std::decay_t<callable_t>>)
                {
                    return callable;
                }
                else
                {
                    return nullptr;
                }
            }

            std::vector<std::pair<const void*, downstream_t>> receivers_;
        };

        template<typename... bases_t>
        struct multi_generator_impl : bases_t...
        {
            using bases_t::operator()...;
            using bases_t::operator+=...;
            using bases_t::operator-=...;
        };
    }

    template<typename... outputs_t>
    struct data_generator : impl::multi_generator_impl<impl::data_generator_impl<outputs_t>...>
    {
        template<typename callable_t,
            concepts::IsInvocableWithAny<callable_t, outputs_t...> = nullptr>
        void operator+=(callable_t&& downstream)
        {
            // Recursively call += for each output type to find exact base/data_generator
            auto callback = [](auto&& base, auto&& downstream){
                base->operator+=(std::forward<decltype(downstream)>(downstream));
            };
            do_for_each_matching_generator<callable_t, outputs_t...>(std::forward<callable_t>(downstream), callback);
        }

        template<typename callable_t,
            concepts::IsInvocableWithAny<callable_t, outputs_t...> = nullptr>
            void operator-=(callable_t&& downstream)
        {
            // Recursively call -= for each output type to find exact base/data_generator
            auto callback = [](auto&& base, auto&& downstream)
            {
                base->operator-=(std::forward<decltype(downstream)>(downstream));
            };
            do_for_each_matching_generator<callable_t, outputs_t...>(std::forward<callable_t>(downstream), callback);
        }

    private:
        template<typename callable_t, typename output_t, typename callback_t>
        void do_for_each_matching_generator(callable_t&& downstream, callback_t&& callback)
        {
            // Register callable for matching output only
            if constexpr (meta::is_invocable_v<callable_t, output_t>)
            {
                // Explicitly call correct base to avoid ambiguity
                using exact_base_t = impl::data_generator_impl<output_t>;
                callback(static_cast<exact_base_t*>(this), std::forward<callable_t>(downstream));
            }
        }

        template<typename callable_t, typename output_t, typename tail_t, typename... tails_t, typename callback_t>
        void do_for_each_matching_generator(callable_t&& downstream, callback_t&& callback)
        {
            do_for_each_matching_generator<callable_t, output_t>(std::forward<callable_t>(downstream), std::forward<callback_t>(callback));
            // Recursively register callable for each (matching) output type
            do_for_each_matching_generator<callable_t, tail_t, tails_t...>(std::forward<callable_t>(downstream), std::forward<callback_t>(callback));
        }
    };
}