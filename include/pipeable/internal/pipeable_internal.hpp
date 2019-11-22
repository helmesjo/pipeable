#pragma once

#include <type_traits>
#include <tuple>
#include <utility>

namespace pipeable
{
    namespace impl
    {
        struct pipe_tag {};
        struct custom_pipeable_tag {};
        struct pipe_interceptor_tag {};
    }

    namespace meta
    {
        template<typename... Ts>
        constexpr bool is_pipe_v = (std::is_base_of_v<impl::pipe_tag, std::decay_t<Ts>> && ...);

        template<typename... Ts>
        constexpr bool is_custom_pipeable_v = (std::is_base_of_v<impl::custom_pipeable_tag, std::decay_t<Ts>> && ...);

        template<typename... Ts>
        constexpr bool is_interceptor_v = (std::is_base_of_v<impl::pipe_interceptor_tag, std::decay_t<Ts>> && ...);
    }

    namespace concepts
    {
        namespace details
        {
            template<std::size_t>
            struct tag;

            template<std::size_t i>
            using tag_t = tag<i>*;
        }

        template<typename... Ts>
        using IsPipe = std::enable_if_t<meta::is_pipe_v<Ts...>, details::tag_t<0>>;

        template<typename... Ts>
        using IsNotPipe = std::enable_if_t<!meta::is_pipe_v<Ts...>, details::tag_t<1>>;

        template<typename... Ts>
        using IsCustomPipeable = std::enable_if_t<meta::is_custom_pipeable_v<Ts...>, details::tag_t<2>>;

        template<typename... Ts>
        using IsNotCustomPipeable = std::enable_if_t<!meta::is_custom_pipeable_v<Ts...>, details::tag_t<3>>;

        template<typename interceptor_t>
        using IsInterceptor = std::enable_if_t<meta::is_interceptor_v<interceptor_t>, details::tag_t<4>>;

        template<typename T>
        using IsNotInterceptor = std::enable_if_t<!meta::is_interceptor_v<T>, details::tag_t<5>>;
    }

    namespace impl
    {
        // Wrapper taking care of invocation of tail + head pair.
        // If tail & head are not interceptors: tail(head(args...));
        // If head is interceptor: head(tail, args...);
        template<typename tail_t, typename head_t>
        struct invoke_pair
        {
            template<typename T1, typename T2>
            invoke_pair(T1&& tail, T2&& head) :
                tail(std::forward<T1>(tail)),
                head(std::forward<T2>(head))
            {
            }

            // If head & tail are callable, invoke as: tail(head(args...))
            template<typename... args_t, typename T = tail_t, typename H = head_t,
                concepts::IsNotInterceptor<T> = nullptr,
                concepts::IsNotInterceptor<H> = nullptr>
            decltype(auto) operator()(args_t&&... args)
            {
                return tail(head(std::forward<args_t>(args)...));
            }

            // If head is interceptor & tail is callable, invoke as: interceptor(tail, args...)
            template<typename... args_t, typename T = tail_t, typename H = head_t,
                concepts::IsInterceptor<H> = nullptr,
                concepts::IsNotInterceptor<T> = nullptr>
            decltype(auto) operator()(args_t&&... args)
            {
                return head(tail, std::forward<args_t>(args)...);
            }

            template<typename... args_t>
            static constexpr bool is_invocable()
            {
                using head_t_ = std::remove_pointer_t<head_t>;
                if constexpr (meta::is_interceptor_v<head_t_>)
                {
                    return std::is_invocable_v<head_t_, tail_t, args_t...>;
                }
                else
                {
                    return std::is_invocable_v<head_t_, args_t...>;
                }
            }

            tail_t tail;
            head_t head;
        };
        // Deduction guide to figure out callable types
        template<typename T1, typename T2>
        invoke_pair(T1&& tail, T2&& head)
            ->
            invoke_pair<T1, T2>;
    }

    namespace invocation
    {
        template<typename... args_t>
        inline constexpr auto make_index_sequence(std::tuple<args_t...> = {})
        {
            return std::index_sequence_for<args_t...>();
        }

        template<typename arg_t>
        constexpr decltype(auto) invoke(arg_t&& arg)
        {
            if constexpr (std::is_invocable_v<decltype(arg)>)
            {
                return std::forward<arg_t>(arg)();
            }
            else
            {
                return std::forward<arg_t>(arg);
            }
        }

        template<typename head_t, typename arg_t,
            concepts::IsNotPipe<head_t> = nullptr>
        constexpr decltype(auto) invoke(head_t&& head, arg_t&& arg);

        template<typename tail_t, typename head_t, typename... rest_t,
            concepts::IsNotPipe<tail_t> = nullptr,
            concepts::IsNotPipe<head_t> = nullptr>
            constexpr decltype(auto) invoke(tail_t&& tail, head_t&& head, rest_t&&... rest);

        template<typename composite_t, std::size_t... indexes, typename... args_t,
            concepts::IsPipe<composite_t> = nullptr>
        inline constexpr decltype(auto) invoke(composite_t&& pipe, std::index_sequence<indexes...>, args_t&&... args)
        {
            // Reverse order to get tail...head which will be composed to: tail(...(head(args)));
            return invoke(std::get<(sizeof...(indexes) - 1 - indexes)>(std::forward<composite_t>(pipe).callables)..., std::forward<args_t>(args)...);
        }

        template<typename composite_t, typename... args_t, 
            concepts::IsPipe<composite_t> = nullptr>
        inline constexpr decltype(auto) invoke(composite_t&& pipe, args_t&&... args)
        {
            return invoke(std::forward<composite_t>(pipe), make_index_sequence(std::forward<composite_t>(pipe).callables), std::forward<args_t>(args)...);
        }
    }

    namespace meta
    {
        namespace details
        {
            struct no_op_callable
            {
                template<typename... args_t>
                void operator()(args_t&&...){}
            };

            template<typename arg_t, typename head_t, typename... tails_t>
            constexpr bool is_invocable_impl()
            {
                if constexpr (std::is_void_v<arg_t>)
                {
                    return std::is_invocable_v<head_t>;
                }
                else
                {
                    using invoker_t = impl::invoke_pair<no_op_callable, std::decay_t<head_t>>;

                    if constexpr (std::is_invocable_v<arg_t>)
                    {
                        using arg_result_t = std::invoke_result_t<arg_t>;
                        return invoker_t::template is_invocable<arg_result_t>();
                    }
                    else
                    {
                        return invoker_t::template is_invocable<arg_t>();
                    }
                }
            }

            template<typename tuple_t, typename arg_t, std::size_t... indexes>
            constexpr bool is_invocable_impl(std::index_sequence<indexes...>)
            {
                return is_invocable_impl<arg_t, std::tuple_element_t<indexes, tuple_t>...>();
            }

            template<typename composite_t, typename arg_t, concepts::IsPipe<composite_t> = nullptr>
            constexpr bool is_invocable()
            {
                using tuples_t = typename std::decay_t<composite_t>::callables_tuple_t;
                using sequence_t = decltype(invocation::make_index_sequence(std::declval<tuples_t>()));
                return is_invocable_impl<tuples_t, arg_t>(sequence_t{});
            }

            template<typename callable_t, typename arg_t = void, concepts::IsNotPipe<callable_t> = nullptr>
            constexpr bool is_invocable()
            {
                return is_invocable_impl<arg_t, callable_t>();
            }
        }

        template<typename T, typename... args_t>
        constexpr bool is_invocable_v = meta::details::is_invocable<T, args_t...>();

        template<typename callable_t, typename... args_t>
        constexpr bool is_invocable_with_any_v = (meta::is_invocable_v<callable_t, args_t> || ...);

        template<typename T, typename args_t, concepts::IsPipe<T> = nullptr> // TODO: Parameter pack should work here, but automatic type deduction fails for args_t on GCC, and complains about IsPipe<T>... FIX!
        using pipe_result_of_t = decltype(invocation::invoke(std::declval<T>(), std::declval<args_t>()));
    }

    namespace concepts
    {
        template<typename receiver_t, typename... args_t>
        using IsInvocable = std::enable_if_t<meta::is_invocable_v<receiver_t, args_t...>, details::tag_t<6>>;

        template<typename callable_t, typename... args_t>
        using IsInvocableWithAny = std::enable_if_t<meta::is_invocable_with_any_v<callable_t, args_t...>, details::tag_t<7>>;
    }

    namespace impl
    {
        // Compile time class holding all callable types, accessor (value/pointer) and qualifiers.
        // Eg. with 'auto pipeline = assembly::compose(callableA, callableB_ptr, const_CallableC_ptr)',
        // decltype(pipeline) is: composite_pipe<callableA_t, callableB_t*, const callableC_t*>
        // Callables are ordered left to right as head...tail.
        template<typename... callables_t>
        struct composite_pipe : impl::pipe_tag
        {
            static_assert(sizeof...(callables_t) > 0, "composite_pipe: 1 or more callables required.");

            using callables_tuple_t = std::tuple<callables_t...>;
            using head_t = std::tuple_element_t<0, callables_tuple_t>;
            using tail_t = std::tuple_element_t<sizeof...(callables_t) - 1, callables_tuple_t>;

            template<typename... Ts, concepts::IsNotPipe<Ts...> = nullptr>
            composite_pipe(Ts&&... callables) :
                callables(std::forward<Ts>(callables)...)
            {
            }
            composite_pipe() = default;
            composite_pipe(const composite_pipe& rhs) = default;
            composite_pipe(composite_pipe&& rhs) = default;

            template<typename T>
            operator T() const
            {
                return [pipe = *this](auto&&... args)
                {
                    return invocation::invoke(pipe, std::forward<decltype(args)>(args)...);
                };
            }

            callables_tuple_t callables;
        };

        // Deduction guide to figure out callable types
        template<typename T, typename... Ts>
        composite_pipe(T&&, Ts&&...)
            ->
            composite_pipe<std::remove_reference_t<T>, std::remove_reference_t<Ts>...>;

        // Wraps a lambda in a new type tagged as an 'interceptor' (used by type traits)
        template<typename callable_base_t>
        struct interceptor : impl::pipe_interceptor_tag, callable_base_t
        {
            interceptor(callable_base_t&& callable) :
                callable_base_t(std::move(callable))
            {}
        };
    }

    namespace assembly
    {
        template<typename head_t, std::size_t... head_indexes, typename callable_t>
        inline constexpr decltype(auto) compose(head_t&& head, std::index_sequence<head_indexes...>, callable_t&& tail)
        {
            return impl::composite_pipe(std::get<head_indexes>(std::forward<decltype(head)>(head).callables)..., tail);
        }

        template<typename callable_t, typename tail_t, std::size_t... tail_indexes>
        inline constexpr decltype(auto) compose(callable_t&& head, tail_t&& tail, std::index_sequence<tail_indexes...>)
        {
            return impl::composite_pipe(std::forward<callable_t>(head), std::get<tail_indexes>(std::forward<decltype(tail)>(tail).callables)...);
        }

        template<typename head_t, std::size_t... head_indexes, typename tail_t, std::size_t... tail_indexes>
        inline constexpr decltype(auto) compose(head_t&& head, std::index_sequence<head_indexes...>, tail_t&& tail, std::index_sequence<tail_indexes...>)
        {
            return impl::composite_pipe(std::get<head_indexes>(std::forward<decltype(head)>(head).callables)..., std::get<tail_indexes>(std::forward<decltype(tail)>(tail).callables)...);
        }

        template<typename head_t, typename tail_t>
        constexpr decltype(auto) compose(head_t&& head, tail_t&& tail)
        {
            // Callable -> Callable
            if constexpr (!meta::is_pipe_v<head_t> && !meta::is_pipe_v<tail_t>)
            {
                return impl::composite_pipe(std::forward<head_t>(head), std::forward<tail_t>(tail));
            }
            // Callable -> Pipe
            else if constexpr (!meta::is_pipe_v<head_t> && meta::is_pipe_v<tail_t>)
            {
                return compose(std::forward<head_t>(head), std::forward<tail_t>(tail), invocation::make_index_sequence(tail.callables));
            }
            // Pipe -> Callable
            else if constexpr (meta::is_pipe_v<head_t> && !meta::is_pipe_v<tail_t>)
            {
                return compose(std::forward<head_t>(head), invocation::make_index_sequence(head.callables), std::forward<tail_t>(tail));
            }
            // Pipe -> Pipe
            else if constexpr (meta::is_pipe_v<head_t> && meta::is_pipe_v<tail_t>)
            {
                return compose(std::forward<head_t>(head), invocation::make_index_sequence(head.callables), std::forward<tail_t>(tail), invocation::make_index_sequence(tail.callables));
            }
        }

        template<typename head_t, typename tail_t, typename... tails_t>
        constexpr decltype(auto) compose(head_t&& head, tail_t&& tail, tails_t&&... tails)
        {
            return compose(compose(std::forward<head_t>(head), std::forward<tail_t>(tail)), std::forward<tails_t>(tails)...);
        }

        template<typename callable_t>
        constexpr auto make_interceptor(callable_t&& callable) 
            -> std::enable_if_t<std::is_rvalue_reference_v<decltype(callable)>, impl::interceptor<std::decay_t<callable_t>>>
        {
            return impl::interceptor(std::move(callable));
        }
    }
}

namespace pipeable
{
    namespace invocation
    {
        template<typename tail_t, typename head_t, typename... rest_t,
            concepts::IsNotPipe<tail_t>,
            concepts::IsNotPipe<head_t>>
        constexpr decltype(auto) invoke(tail_t&& tail, head_t&& head, rest_t&&... rest)
        {
            if constexpr (std::is_pointer_v<std::decay_t<tail_t>>)
            {
                return invoke(*std::forward<tail_t>(tail), std::forward<head_t>(head), std::forward<rest_t>(rest)...);
            }
            else if constexpr (std::is_pointer_v<std::decay_t<head_t>>)
            {
                return invoke(std::forward<tail_t>(tail), *std::forward<head_t>(head), std::forward<rest_t>(rest)...);
            }
            else
            {
                return invoke(impl::invoke_pair(std::forward<tail_t>(tail), std::forward<head_t>(head)), std::forward<rest_t>(rest)...);
            }
        }

        template<typename head_t, typename arg_t,
            concepts::IsNotPipe<head_t>>
        constexpr decltype(auto) invoke(head_t&& head, arg_t&& arg)
        {
            if constexpr (std::is_pointer_v<std::decay_t<head_t>>)
            {
                return invoke(*std::forward<head_t>(head), std::forward<arg_t>(arg));
            }
            else
            {
                return std::forward<head_t>(head)(invoke(std::forward<arg_t>(arg)));
            }
        }
    }
}