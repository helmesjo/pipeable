#pragma once

#include <pipeable/pipeable.hpp>
#include <functional>

namespace pipeable
{
    template<typename... outputs_t>
    struct data_generator : impl::custom_pipeable_tag
    {
        void operator()(const outputs_t&... args)
        {
            downstream_(args...);
        }

        template<typename callable_t>
        data_generator& operator>>=(callable_t&& downstream)
        {
            downstream_ = [downstream = std::forward<callable_t>(downstream)](auto&&... args) mutable
            {
                invocation::invoke(std::forward<callable_t>(downstream), std::forward<decltype(args)>(args)...);
            };

            return *this;
        }

    private:
        std::function<void(const outputs_t&...)> downstream_ = [](const auto&...){};
    };
}