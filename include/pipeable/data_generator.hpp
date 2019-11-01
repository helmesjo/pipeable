#pragma once

#include <pipeable/pipeable.hpp>
#include <functional>

namespace pipeable
{
    template<typename... outputs_t>
    struct data_generator : impl::custom_pipeable_tag
    {
        template<typename... args_t>
        void operator()(args_t&&... args)
        {
            downstream_(std::forward<args_t>(args)...);
        }

        template<typename callable_t>
        data_generator& operator>>=(callable_t&& downstream)
        {
            downstream_ = [downstream = std::forward<callable_t>(downstream)](const outputs_t&... args) mutable
            {
                std::tuple(args...) >>= unpack >>= downstream;
            };

            return *this;
        }

    private:
        std::function<void(const outputs_t&...)> downstream_;
    };
}