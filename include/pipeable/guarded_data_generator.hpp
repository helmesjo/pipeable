#pragma once

#include <pipeable/data_generator.hpp>
#include <memory>
#include <mutex>
#include <vector>

namespace pipeable
{
    namespace impl
    {
        template<typename T>
        struct threadsafe_receivers
        {
            template<typename callback_t>
            void for_each(callback_t&& callback) const
            {
                auto tmp = std::atomic_load(&receivers_);
                for (auto&& downstream : *tmp)
                {
                    std::forward<callback_t>(callback)(std::forward<decltype(downstream)>(downstream));
                }
            }

            template<typename callback_t>
            void modify_list(callback_t&& callback)
            {
                std::scoped_lock lock{ mutex_ };
                container_t copy = *receivers_;
                std::forward<callback_t>(callback)(copy);
                std::atomic_store(&receivers_, std::make_shared<container_t>(std::move(copy)));
            }

        private:
            using container_t = std::vector<T>;

            std::mutex mutex_;
            std::shared_ptr<container_t> receivers_ = std::make_shared<container_t>();
        };

        struct thread_safe {};
        template<typename receiver_t>
        struct receivers_t<receiver_t, thread_safe>
        {
            using type_t = threadsafe_receivers<receiver_t>;
        };
    }

    template<typename... outputs_t>
    struct guarded_data_generator : impl::multi_generator<impl::thread_safe, outputs_t...>
    {};
}