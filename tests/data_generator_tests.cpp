#include <pipeable/pipeable.hpp>
#include <pipeable/data_generator.hpp>

#include <catch2/catch.hpp>
#include <variant>

using namespace pipeable;

namespace
{
    struct int_to_int
    {
        int operator()(int val)
        {
            receivedValue = true;
            return val;
        }
        bool receivedValue = false;
    };
    struct int_and_string_receiver
    {
        int receivedInt = 0;
        std::string receivedStr = "";
        void operator()(int val)
        {
            receivedInt = val;
        }
        void operator()(const std::string& val)
        {
            receivedStr = val;
        }
    };
}

SCENARIO("Compose pipelines with a data generator")
{
    GIVEN("a data generator")
    {
        data_generator<int> generator;
        WHEN("it is piped to a receiver")
        {
            int_to_int receiver;
            generator >>= &receiver;
            THEN("no data is forwarded automatically")
            {
                REQUIRE(receiver.receivedValue == false);
            }
            AND_WHEN("generator is invoked with data")
            {
                generator(1);
                THEN("it is forwarded to the downstream pipeline")
                {
                    REQUIRE(receiver.receivedValue == true);
                }
            }
        }
    }
    GIVEN("a data generator outputting variant<x, y>")
    {
        data_generator<std::variant<int, std::string>> generator;

        WHEN("it is piped as: generator >>= visit >>= receiver")
        {
            int_and_string_receiver receiver;
            generator >>= visit >>= &receiver;
            AND_WHEN("generator is invoked with x")
            {
                generator(1);
                THEN("receiver::operator(x) is invoked")
                {
                    REQUIRE(receiver.receivedInt == 1);
                }
            }
            AND_WHEN("generator is invoked with y")
            {
                generator("hello");
                THEN("receiver::operator(y) is invoked")
                {
                    REQUIRE(receiver.receivedStr == "hello");
                }
            }
        }
    }
}