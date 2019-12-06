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
        WHEN("generator is piped to receiver")
        {
            int_to_int receiver;
            generator += &receiver;
            THEN("no data is forwarded automatically")
            {
                REQUIRE(receiver.receivedValue == false);
            }
            THEN("data is forwarded to the downstream pipeline")
            {
                generator(1);
                REQUIRE(receiver.receivedValue == true);
            }
            AND_WHEN("receiver is deregistered from generator")
            {
                generator -= &receiver;
                THEN("data is no longer forwarded")
                {
                    receiver.receivedValue = false;
                    generator(1);
                    REQUIRE(receiver.receivedValue == false);
                }
            }
        }
    }
    GIVEN("a data generator outputting variant<x, y>")
    {
        data_generator<std::variant<int, std::string>> generator;

        WHEN("it is piped as: generator += visit >>= receiver")
        {
            int_and_string_receiver receiver;
            generator += visit >>= &receiver;
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

SCENARIO("multi-output generator")
{
    GIVEN("a generator outputting int & string")
    {
        data_generator<int, std::string> multiGenerator;

        WHEN("piped to receiver callable with int")
        {
            int receivedInt = 0;
            const auto receiver = [&](int val) { receivedInt = val; };

            multiGenerator += &receiver;
            THEN("it receives the generated int")
            {
                multiGenerator(1);
                REQUIRE(receivedInt == 1);
            }
        }
        WHEN("piped to receiver callable with string")
        {
            std::string receivedStr = "";
            const auto receiver = [&](std::string val) { receivedStr = val; };

            multiGenerator += &receiver;
            THEN("it receives the generated string")
            {
                multiGenerator("1");
                REQUIRE(receivedStr == "1");
            }
        }

        WHEN("piped to receiver callable with int and string")
        {
            struct receiver_t
            {
                int receivedInt = 0;
                std::string receivedStr = "";
                void operator()(int val) { receivedInt = val; }
                void operator()(std::string val) { receivedStr = val; }
                void operator()(std::tuple<>) { } // dummy
            } receiver;

            multiGenerator += &receiver;

            THEN("it receives the generated int")
            {
                multiGenerator(1);
                REQUIRE(receiver.receivedInt == 1);
            }
            THEN("it receives the generated string")
            {
                multiGenerator("1");
                REQUIRE(receiver.receivedStr == "1");
            }
        }

        WHEN("piped to r-value receiver containing shared_ptr, callable with both int and string")
        {
            auto wasPtrEmpty = false;
            multiGenerator += [&wasPtrEmpty, ptr = std::make_shared<int>()](const auto& value) mutable
            {
                wasPtrEmpty = ptr == nullptr;
            };

            THEN("contained shared_ptr wasn't moved from (is empty)")
            {
                multiGenerator(1);
                REQUIRE(!wasPtrEmpty);
            }
            THEN("contained shared_ptr wasn't moved from (is empty)")
            {
                multiGenerator("1");
                REQUIRE(!wasPtrEmpty);
            }
        }
    }
}

SCENARIO("Chaining generators")
{
    GIVEN("A generator outputting int")
    {
        data_generator<int> gen1;
        WHEN("chained to another generator outputting int")
        {
            data_generator<int> gen2;
            gen1 += &gen2;
            bool didReceiveData = false;
            auto receiver = [&](int) { didReceiveData = true; };
            gen2 += &receiver;
            THEN("invoking first generator will invoke second")
            {
                gen1(1);
                REQUIRE(didReceiveData);
            }
        }
    }
}