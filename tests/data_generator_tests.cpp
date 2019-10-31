#include <pipeable/pipeable.hpp>
#include <pipeable/data_generator.hpp>

#include <catch2/catch.hpp>

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
            AND_WHEN("data is piped to generator")
            {
                1 >>= generator;
                THEN("it is forwarded to the downstream pipeline")
                {
                    REQUIRE(receiver.receivedValue == true);
                }
            }
        }
    }
}