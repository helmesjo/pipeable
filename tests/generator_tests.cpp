#include <pipeable/pipeable.hpp>
#include <pipeable/generator.hpp>

#include <catch2/catch.hpp>
#include <type_traits>
#include <string>
#include <chrono>

using namespace pipeable;
using a_clock_t = std::chrono::system_clock;
using time_point_t = std::chrono::time_point<a_clock_t>;

namespace
{
    template<typename T>
    struct my_generator final : public generator<T>
    {
        my_generator(std::initializer_list<T> vals) :
            vals_(vals.begin(), vals.end())
        {}

        std::optional<T> next() override
        {
            return current_ < vals_.size() ? std::optional<T>(vals_[current_++]) : std::nullopt;
        }

        std::vector<T> vals_;

    private:
        std::size_t current_ = 0;
    };

    struct int_to_int
    {
        int operator()(int val)
        {
            callTime = a_clock_t::now();
            return val;
        }

        time_point_t callTime;
    };
    struct int_to_string
    {
        std::string operator()(int val)
        {
            callTime = a_clock_t::now();
            return std::to_string(val);
        }
        time_point_t callTime;
    };

    template<typename T>
    struct hello;
}

SCENARIO("Compose pipeline stages with first stage generating data")
{
    GIVEN("a generator")
    {
        THEN("it is iterable")
        {
            REQUIRE(pipeable::type::is_iterable_v<generator<int>>);
        }

        my_generator<int> myGenerator = { 1, 2, 3 };

        WHEN("iterated")
        {
            std::vector<int> vals;
            for (auto val : myGenerator)
            {
                vals.push_back(val);
            }
            THEN("each value is iterated")
            {
                REQUIRE(vals.size() == 3);
                REQUIRE(vals[0] == 1);
                REQUIRE(vals[1] == 2);
                REQUIRE(vals[2] == 3);
            }
        }

        WHEN("piped as: generator >>= for_each >>= receiver")
        {
            std::vector<std::string> newValues;
            auto callable = [&](std::string val)
            {
                newValues.push_back(val);
            };

            myGenerator >>= for_each >>= int_to_int() >>= int_to_string() >>= callable;

            THEN("receiver is invoked once for each element")
            {
                REQUIRE(newValues.size() == 3);
                REQUIRE(newValues[0] == "1");
                REQUIRE(newValues[1] == "2");
                REQUIRE(newValues[2] == "3");
            }
        }
    }
}