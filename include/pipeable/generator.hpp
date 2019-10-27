#pragma once

#include <iterator>
#include <optional>
#include <type_traits>

namespace pipeable
{
    /*
    Provides basic functionality for creating a "data source", which
    can be iterated to extract data until no value is returned (empty optional).
    Prefer marking derived classes as "final" to allow devirtualization.
    */
    template<typename T>
    struct generator
    {
        virtual std::optional<T> next() = 0;

        struct iterator
        {
            using iterator_category = std::input_iterator_tag;
            using value_type = std::decay_t<T>;
            using reference = value_type const&;
            using pointer = value_type const*;
            using difference_type = ptrdiff_t;

            iterator(generator& gen, bool end) : 
                generator_(gen)
            {
                if (!end)
                {
                    ++(*this);
                }
            }
            bool operator== (iterator const& other) const
            {
                return this->current_ == other.current_;
            }
            bool operator!= (iterator const& other) const
            {
                return !(*this == other);
            }
            explicit operator bool() const
            {
                return !current_.has_value();
            }
            reference operator*() const
            {
                return current_.value();
            }
            pointer operator->() const
            {
                return &current_.value();
            }

            iterator& operator++()
            {
                current_ = generator_.next();
                return *this;
            }

        private:
            generator& generator_;
            std::optional<T> current_;
        };

        iterator begin()
        {
            return iterator(*this, false);
        }
        iterator end()
        {
            return iterator(*this, true);
        }
    };
}