#pragma once

#include <pipeable/pipeable.hpp>
#include <functional>
#include <vector>

namespace pipeable
{
    namespace concepts
    {
        template<typename from_t, typename to_t>
        using IsConvertible = std::enable_if_t<std::is_convertible_v<from_t, to_t>, details::tag_t<0>>;
    }

    namespace impl
    {
        template<typename T>
        struct non_threadsafe_receivers : std::vector<T>
        {
            template<typename callback_t>
            void for_each(callback_t&& callback) const
            {
                for (auto&& downstream : *this)
                {
                    std::forward<callback_t>(callback)(std::forward<decltype(downstream)>(downstream));
                }
            }

            template<typename callback_t>
            void modify_list(callback_t&& callback)
            {
                std::forward<callback_t>(callback)(*this);
            }
        };

        template<typename receiver_t, typename>
        struct receivers_t
        {
        };

        struct non_thread_safe{};
        template<typename receiver_t>
        struct receivers_t<receiver_t, non_thread_safe>
        {
            using type_t = non_threadsafe_receivers<receiver_t>;
        };

        template<typename output_t, typename collection_type_tag_t>
        struct data_generator_impl : impl::custom_pipeable_tag
        {
            using downstream_t = std::function<void(output_t)>;

            template<typename arg_t,
                concepts::IsConvertible<arg_t, output_t> = nullptr>
            void operator()(arg_t&& arg) const
            {
                receivers_.for_each([&](auto&& downstream) {
                    if constexpr (std::is_rvalue_reference_v<output_t>)
                    {
                        std::forward<decltype(downstream)>(downstream).second(std::move(arg));
                    }
                    else
                    {
                        // Intentionally don't forward here, since if we have multiple receivers and the value is "moved from" 
                        // into the first, remaining receivers will (if it can be moved from) not get the value.
                        std::forward<decltype(downstream)>(downstream).second(arg);
                    }
                });
            }

            template<typename callable_t,
                concepts::IsInvocable<callable_t, output_t> = nullptr>
            void operator+=(callable_t&& downstream)
            {
                auto id = identifier(downstream);
                downstream_t receiverCall = [downstream = downstream](auto&& arg) mutable
                {
                    invocation::invoke(FWD(downstream), FWD(arg));
                };
                receivers_.modify_list([&](auto& receivers) {
                    receivers.emplace_back(id, receiverCall);
                });
            }

            template<typename callable_t,
                concepts::IsInvocable<callable_t, output_t> = nullptr>
            void operator-=(callable_t&& downstream)
            {
                receivers_.modify_list([&](auto& receivers){
                    receivers.erase(std::remove_if(receivers.begin(), receivers.end(), [addr = identifier(downstream)](auto&& receiver) {
                        return receiver.first == addr;
                    }), receivers.end());
                });
            }

        private:

            typename receivers_t<std::pair<const void*, downstream_t>, collection_type_tag_t>::type_t receivers_;

            template<typename callable_t>
            static constexpr const void* identifier(const callable_t& callable)
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
        };

        template<typename... bases_t>
        struct multi_generator_impl : bases_t...
        {
            using bases_t::operator()...;
            using bases_t::operator+=...;
            using bases_t::operator-=...;
        };

        template<typename threading_t, typename... outputs_t>
        struct multi_generator : impl::multi_generator_impl<impl::data_generator_impl<outputs_t, threading_t>...>
        {
            template<typename callable_t,
                concepts::IsInvocableWithAny<callable_t, outputs_t...> = nullptr>
                void operator+=(callable_t&& downstream)
            {
                // Recursively call += for each output type to find exact base/data_generator
                auto callback = [](auto&& base, auto&& downstream) {
                    base->operator+=(FWD(downstream));
                };
                do_for_each_matching_generator<callable_t, outputs_t...>(FWD(downstream), callback);
            }

            template<typename callable_t,
                concepts::IsInvocableWithAny<callable_t, outputs_t...> = nullptr>
                void operator-=(callable_t&& downstream)
            {
                // Recursively call -= for each output type to find exact base/data_generator
                auto callback = [](auto&& base, auto&& downstream)
                {
                    base->operator-=(FWD(downstream));
                };
                do_for_each_matching_generator<callable_t, outputs_t...>(FWD(downstream), callback);
            }

        private:
            template<typename callable_t, typename output_t, typename callback_t>
            void do_for_each_matching_generator(callable_t&& downstream, callback_t&& callback)
            {
                // Register callable for matching output only
                if constexpr (meta::is_invocable_v<callable_t, output_t>)
                {
                    // Explicitly call correct base to avoid ambiguity
                    using exact_base_t = impl::data_generator_impl<output_t, threading_t>;
                    callback(static_cast<exact_base_t*>(this), FWD(downstream));
                }
            }

            template<typename callable_t, typename output_t, typename tail_t, typename... tails_t, typename callback_t>
            void do_for_each_matching_generator(callable_t&& downstream, callback_t&& callback)
            {
                do_for_each_matching_generator<callable_t, output_t>(FWD(downstream), FWD(callback));
                // Recursively register callable for each (matching) output type
                do_for_each_matching_generator<callable_t, tail_t, tails_t...>(FWD(downstream), FWD(callback));
            }
        };
    }

    template<typename... outputs_t>
    struct data_generator : impl::multi_generator<impl::non_thread_safe, outputs_t...>
    {};
}