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
    struct data_source
    {
        virtual std::optional<T> next() = 0;

        struct iterator
        {
            using iterator_category = std::input_iterator_tag;
            using value_type = std::decay_t<T>;
            using reference = value_type const&;
            using pointer = value_type const*;
            using difference_type = ptrdiff_t;

            iterator(data_source& source, bool end) :
                source_(source)
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
                current_ = source_.next();
                return *this;
            }

        private:
            data_source& source_;
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