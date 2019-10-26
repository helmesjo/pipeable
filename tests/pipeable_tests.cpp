#include <pipeable/pipeable.hpp>

#include <catch2/catch.hpp>

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

using namespace pipeable;
using pipeable::operator>>=;

using a_clock_t = std::chrono::system_clock;
using time_point_t = std::chrono::time_point<a_clock_t, std::chrono::nanoseconds>;

namespace
{
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
    struct string_to_char
    {
        const char* operator()(std::string val)
        {
            callTime = a_clock_t::now();
            return (val_ = val).c_str();
        }
        time_point_t callTime;

    private:
        std::string val_;
    };
    struct char_to_int
    {
        int operator()(const char* val)
        {
            callTime = a_clock_t::now();
            return std::stoi(std::string(val));
        }
        time_point_t callTime;
    };

    struct int_and_string_to_int
    {
        int operator()(int val)
        {
            return val;
        }
        int operator()(const std::string& val)
        {
            return std::stoi(val);
        }
    };

    template<typename T>
    struct hello;
}

SCENARIO("Pipeline type traits")
{
    GIVEN("a callable with signature: int(int)")
    {
        THEN("it is invocable with int")
        {
            REQUIRE(meta::is_invocable_v<int_to_int, int>);
        }
    }
    GIVEN("a callable without arguments and no return type")
    {
        constexpr auto no_arg_no_return_callable = []() {};
        THEN("it is invocable without arguments")
        {
            REQUIRE(meta::is_invocable_v<decltype(no_arg_no_return_callable)>);
        }
    }
    GIVEN("a callable without arguments with return type")
    {
        constexpr auto no_arg_with_return_callable = []() -> int { return 1; };
        THEN("it is invocable without arguments")
        {
            REQUIRE(meta::is_invocable_v<decltype(no_arg_with_return_callable)>);
        }
    }
    GIVEN("a pipeline composed as: auto pipeline = int_to_int() >>= int_to_int()")
    {
        auto pipeline = assembly::compose(int_to_int(), int_to_int());
        THEN("the composed type is invocable with int")
        {
            REQUIRE(meta::is_invocable_v<decltype(pipeline), int>);
        }
        THEN("the composed type is invocable with callable with signature: int(void)")
        {
            constexpr auto no_arg_with_return_callable = []() -> int { return 1; };
            REQUIRE(meta::is_invocable_v<decltype(pipeline), decltype(no_arg_with_return_callable)>);
        }
        THEN("the result of invoking pipeline is int")
        {
            REQUIRE(std::is_same_v<int, meta::pipe_result_of_t<decltype(pipeline), int>>);
        }
    }
    GIVEN("a pipeline composed as: auto pipeline = int_to_int() >>= int_to_int()")
    {
        auto pipeline = assembly::compose(int_to_int(), int_to_int());
        THEN("the composed type is invocable with int")
        {
            REQUIRE(meta::is_invocable_v<decltype(pipeline), int>);
        }
        THEN("the result of invoking pipeline is int")
        {
            REQUIRE(std::is_same_v<int, meta::pipe_result_of_t<decltype(pipeline), int>>);
        }
    }
    GIVEN("a pipeline composed as: auto pipeline = int_to_int() >>= int_to_string()")
    {
        auto pipeline = assembly::compose(int_to_int(), int_to_string());
        THEN("the composed type is invocable with int")
        {
            REQUIRE(meta::is_invocable_v<decltype(pipeline), int>);
        }
        THEN("the result of invoking pipeline is string")
        {
            REQUIRE(std::is_same_v<std::string, meta::pipe_result_of_t<decltype(pipeline), int>>);
        }
    }
    GIVEN("a pipeline composed as: auto pipeline = int_to_string() >>= string_to_char()")
    {
        auto pipeline = assembly::compose(int_to_string(), string_to_char());
        THEN("the composed type is invocable with int")
        {
            REQUIRE(meta::is_invocable_v<decltype(pipeline), int>);
        }
        THEN("the result of invoking pipeline is const char*")
        {
            REQUIRE(std::is_same_v<const char*, meta::pipe_result_of_t<decltype(pipeline), int>>);
        }
    }
    GIVEN("a pipeline composed as: auto pipeline = int_to_string() >>= string_to_char() >>= char_to_int()")
    {
        auto pipeline = assembly::compose(int_to_string(), string_to_char(), char_to_int());
        THEN("the composed type is invocable with int")
        {
            REQUIRE(meta::is_invocable_v<decltype(pipeline), int>);
        }
        THEN("the result of invoking pipeline is int")
        {
            REQUIRE(std::is_same_v<int, meta::pipe_result_of_t<decltype(pipeline), int>>);
        }
    }
    GIVEN("a pipeline composed as: auto pipeline = int_and_string_to_int() >>= int_to_int()")
    {
        auto pipeline = assembly::compose(int_and_string_to_int(), int_to_int());
        THEN("the composed type is invocable with int")
        {
            REQUIRE(meta::is_invocable_v<decltype(pipeline), int>);
        }
        THEN("the composed type is invocable with string")
        {
            REQUIRE(meta::is_invocable_v<decltype(pipeline), std::string>);
        }
        THEN("the result of invoking pipeline is int")
        {
            REQUIRE(std::is_same_v<int, meta::pipe_result_of_t<decltype(pipeline), int>>);
        }
    }
    GIVEN("a pipeline composed as: auto pipeline = int_to_string() >>= lambda_callable_string_to_int() >>= int_to_int()")
    {
        auto lambda_callable_string_to_int = [](const std::string& val)
        {
            return std::stoi(val);
        };

        auto pipeline = assembly::compose(int_to_string(), lambda_callable_string_to_int, int_to_int());
        THEN("the composed type is invocable with int")
        {
            REQUIRE(meta::is_invocable_v<decltype(pipeline), int>);
        }
        THEN("the result of invoking pipeline is int")
        {
            REQUIRE(std::is_same_v<int, meta::pipe_result_of_t<decltype(pipeline), int>>);
        }
    }
    GIVEN("a pipeline composed as: auto pipeline = lambdaInterceptorStringToInt() >>= int_to_int()")
    {
        auto lambda_interceptor_string_to_int = assembly::make_interceptor([](auto&& pipeline, const std::string& val)
        {
            return pipeline(std::stoi(val));
        });
        auto pipeline = assembly::compose(lambda_interceptor_string_to_int, int_to_int());

        THEN("the composed type is invocable with string")
        {
            REQUIRE(meta::is_invocable_v<decltype(pipeline), std::string>);
        }
        THEN("the result of invoking pipeline is int")
        {
            REQUIRE(std::is_same_v<int, meta::pipe_result_of_t<decltype(pipeline), std::string>>);
        }
    }
    GIVEN("a pipeline composed as: auto pipeline = lambdaInterceptorStringToVoid() >>= int_to_int()")
    {
        auto lambda_interceptor_int_to_void = assembly::make_interceptor([](auto&& pipeline, int val) -> void
        {
            pipeline(val);
        });
        auto pipeline = assembly::compose(lambda_interceptor_int_to_void, int_to_int());

        THEN("the composed type is invocable with int")
        {
            REQUIRE(meta::is_invocable_v<decltype(pipeline), int>);
        }
        THEN("the result of invoking pipeline is void")
        {
            REQUIRE(std::is_same_v<void, meta::pipe_result_of_t<decltype(pipeline), int>>);
        }
    }
    GIVEN("a pipeline composed as: auto pipeline = lambda_interceptor_take_int_return_string() >>= int_to_int() >>= int_to_string()")
    {
        auto lambda_interceptor_take_int_return_string = assembly::make_interceptor([](auto&& pipeline, int val) -> std::string
        {
            return pipeline(val);
        });
        auto pipeline = assembly::compose(lambda_interceptor_take_int_return_string, int_to_int(), int_to_string()); 

        THEN("the composed type is invocable with int")
        {
            REQUIRE(meta::is_invocable_v<decltype(pipeline), int>);
        }
        THEN("the result of invoking pipeline is void")
        {
            REQUIRE(std::is_same_v<std::string, meta::pipe_result_of_t<decltype(pipeline), int>>);
        }
    }
    GIVEN("a pipeline composed as: auto pipeline = int_to_string() >>= lambda_interceptor_string_to_int() >>= int_to_int()")
    {
        auto lambda_interceptor_string_to_int = assembly::make_interceptor([](auto&& pipeline, const std::string& val)
        {
            return pipeline(std::stoi(val));
        });
        auto pipeline = assembly::compose(int_to_string(), lambda_interceptor_string_to_int, int_to_int());

        THEN("the interceptor is detected if arg is string")
        {
            REQUIRE(meta::is_interceptor_v<decltype(lambda_interceptor_string_to_int)>);
        }
        THEN("the interceptor is detected if arg is callable returning string for given arg (string str = callable(arg))")
        {
            REQUIRE(meta::is_interceptor_v<decltype(lambda_interceptor_string_to_int)>);
        }
        THEN("the composed type is invocable with int")
        {
            REQUIRE(meta::is_interceptor_v<decltype(lambda_interceptor_string_to_int)>);
            REQUIRE(meta::is_invocable_v<decltype(pipeline), int>);
        }
    }
}
SCENARIO("Compose callables")
{
    GIVEN("two callables with same input & output type")
    {
        WHEN("composed")
        {
            auto composed = assembly::compose(int_to_int(), int_to_int());

            THEN("head of composite type is same as left hand callable")
            {
                REQUIRE(std::is_same_v<decltype(composed)::head_t, int_to_int>);
            }
            THEN("tail of composite type is same as right hand callable")
            {
                REQUIRE(std::is_same_v<decltype(composed)::tail_t, int_to_int>);
            }
        }
    }
    GIVEN("two callables with different input & output type")
    {
        WHEN("composed")
        {
            auto composed = assembly::compose(int_to_int(), int_to_string());
            THEN("head of composite type is same as left hand callable")
            {
                REQUIRE(std::is_same_v<decltype(composed)::head_t, int_to_int>);
            }
            THEN("tail of composite type is same as right hand callable")
            {
                REQUIRE(std::is_same_v<decltype(composed)::tail_t, int_to_string>);
            }
        }
    }
    GIVEN("left hand composite type and right hand callable")
    {
        auto composed = assembly::compose(int_to_int(), int_to_string());
        WHEN("composed")
        {
            auto newComposed = assembly::compose(composed, string_to_char());
            THEN("head of new composite type is same as head of old composite type")
            {
                REQUIRE(std::is_same_v<decltype(newComposed)::head_t, decltype(composed)::head_t>);
            }
            THEN("tail of new composite type is same as right hand callable")
            {
                REQUIRE(std::is_same_v<decltype(newComposed)::tail_t, string_to_char>);
            }
        }
    }
    GIVEN("left hand callable and right hand composite type")
    {
        auto composed = assembly::compose(int_to_int(), int_to_string());
        WHEN("composed")
        {
            auto newComposed = assembly::compose(char_to_int(), composed);
            THEN("head of new composite type is same as left hand callable")
            {
                REQUIRE(std::is_same_v<decltype(newComposed)::head_t, char_to_int>);
            }
            THEN("tail of new composite type is same as right hand composite type")
            {
                REQUIRE(std::is_same_v<decltype(newComposed)::tail_t, decltype(composed)::tail_t>);
            }
        }
    }
    GIVEN("two composite types")
    {
        auto composed1 = assembly::compose(int_to_int(), int_to_string());
        auto composed2 = assembly::compose(string_to_char(), char_to_int());
        WHEN("composed")
        {
            auto newComposed = assembly::compose(composed1, composed2);
            THEN("head of new composite type is same as head of left hand composite type")
            {
                REQUIRE(std::is_same_v<decltype(newComposed)::head_t, decltype(composed1)::head_t>);
            }
            THEN("tail of new composite type is same as tail of right hand composite type")
            {
                REQUIRE(std::is_same_v<decltype(newComposed)::tail_t, decltype(composed2)::tail_t>);
            }
        }
    }
}

SCENARIO("compose const reference and const pointer to callables")
{
    GIVEN("left hand non-const callable and right hand const callable")
    {
        const auto constCallable = int_to_int();
        WHEN("composed")
        {
            auto composite = assembly::compose(int_to_int(), constCallable);
            THEN("head of composed is non-const")
            {
                REQUIRE(!std::is_const_v<decltype(composite)::head_t>);
            }
            THEN("tail of composed is const")
            {
                REQUIRE(std::is_const_v<decltype(composite)::tail_t>);
            }
            THEN("tail of composed is not reference")
            {
                REQUIRE(!std::is_reference_v<decltype(composite)::tail_t>);
            }
        }
    }
    GIVEN("left hand callable and right hand non-const pointer to callable")
    {
        auto callable = int_to_int();
        WHEN("composed")
        {
            auto composite = assembly::compose(int_to_int(), &callable);
            THEN("tail of composed is pointer type")
            {
                REQUIRE(std::is_pointer_v<decltype(composite)::tail_t>);
            }
            THEN("tail of composed is non-const")
            {
                REQUIRE(!std::is_const_v<std::remove_pointer_t<decltype(composite)::tail_t>>);
            }
        }
    }
    GIVEN("left hand callable and right hand const pointer to callable")
    {
        const auto callable = int_to_int();
        WHEN("composed")
        {
            auto composite = assembly::compose(int_to_int(), &callable);
            THEN("tail of composed is pointer type")
            {
                REQUIRE(std::is_pointer_v<decltype(composite)::tail_t>);
            }
            THEN("tail of composed is const")
            {
                REQUIRE(std::is_const_v<std::remove_pointer_t<decltype(composite)::tail_t>>);
            }
        }
    }
    GIVEN("left hand non-const pointer to callable and right hand const pointer to callable")
    {
        auto callable1 = int_to_int();
        const auto callable2 = int_to_int();
        WHEN("composed")
        {
            auto composite = assembly::compose(&callable1, &callable2);
            THEN("head of composed is pointer type")
            {
                REQUIRE(std::is_pointer_v<decltype(composite)::head_t>);
            }
            THEN("tail of composed is non-const")
            {
                REQUIRE(!std::is_const_v<std::remove_pointer_t<decltype(composite)::head_t>>);
            }
            THEN("tail of composed is pointer type")
            {
                REQUIRE(std::is_pointer_v<decltype(composite)::tail_t>);
            }
            THEN("tail of composed is const")
            {
                REQUIRE(std::is_const_v<std::remove_pointer_t<decltype(composite)::tail_t>>);
            }
        }
    }
}

SCENARIO("Invoke composed callables")
{
    GIVEN("callable accepting an rvalue")
    {
        int receivedValue = 0;
        auto callable = [&](int&& val)
        {
            receivedValue = std::move(val);
        };
        WHEN("an rvalue is piped to the callable")
        {
            1 >>= callable;
            THEN("it is invoked")
            {
                REQUIRE(receivedValue == 1);
            }
        }
    }
    GIVEN("a pipeline of callables")
    {
        auto callable1 = int_to_int();
        auto callable2 = int_to_string();

        auto pipeline = assembly::compose(&callable1, &callable2);
        WHEN("pipe is invoked")
        {
            std::string result = invocation::invoke(pipeline, 1);
            THEN("callables are invoked in correct order")
            {
                REQUIRE(callable1.callTime < callable2.callTime);
            }
            THEN("result is correct transformed value")
            {
                REQUIRE(result == "1");
            }
        }
    }
    GIVEN("a pipeline composed with compose-operator")
    {
        auto pipe = int_to_int() >>= int_to_string() >>= string_to_char() >>= char_to_int();

        WHEN("pipeline is invoked")
        {
            int result = invocation::invoke(pipe, 1);
            THEN("result is correct transformed value")
            {
                REQUIRE(result == 1);
            }
        }
        WHEN("input is piped to pipeline")
        {
            int result = 1 >>= pipe;
            THEN("result is correct transformed value")
            {
                REQUIRE(result == 1);
            }
        }
    }
    GIVEN("a parameterless callable returning value")
    {
        auto return_one = []() -> int { return 2; };
        WHEN("piped to pipeline accepting input of returned type")
        {
            std::string result = return_one >>= int_to_string();
            THEN("it is invoked and result forwarded to downstream pipeline")
            {
                REQUIRE(result == "2");
            }
        }
    }
}
SCENARIO("Implicit conversion")
{
    auto callable1 = int_to_int();
    auto callable2 = int_to_string();
    auto pipeline = assembly::compose(&callable1, &callable2);

    GIVEN("a pipeline is assigned to a type constructible with a callable")
    {
        std::function<std::string(int)> wrapper = pipeline;

        WHEN("invoking the wrapper")
        {
            std::string result = wrapper(1);
            THEN("the pipeline is invoked")
            {
                REQUIRE(callable1.callTime < callable2.callTime);
            }
            THEN("expected result is returned")
            {
                REQUIRE(result == "1");
            }
        }
    }
}
SCENARIO("custom pipeline interceptors")
{
    GIVEN("an interceptor taking first arg as 'auto&& callable' and rest as expected input")
    {
        auto interceptor = assembly::make_interceptor([](auto&& tailPipeline, int val)
        {
            return tailPipeline(val + 1);
        });

        WHEN("a pipeline composed as: auto pipeline = int_to_int() >>= interceptor >>= int_to_string()")
        {
            auto pipeline = int_to_int() >>= interceptor >>= int_to_string();
            AND_WHEN("pipeline is invoked")
            {
                std::string result = 1 >>= pipeline; //interceptor( int_to_string(...), int_to_int(...) );
                THEN("the custom interceptor handles invocation of downstream callables")
                {
                    REQUIRE(result == "2");
                }
            }
        }
        WHEN("a pipeline composed as: auto pipeline = int_to_int() >>= interceptor >>= interceptor >>= int_to_string()")
        {
            auto pipeline = int_to_int() >>= interceptor >>= interceptor >>= int_to_string();
            AND_WHEN("pipeline is invoked")
            {
                std::string result = 1 >>= pipeline; //interceptor( interceptor(int_to_string(...), ...), int_to_int(...) );
                THEN("the custom interceptor handles invocation of downstream callables")
                {
                    REQUIRE(result == "3");
                }
            }
        }
        WHEN("a pipeline composed as: auto pipeline = int_to_int() >>= interceptor >>= interceptor >>= int_to_string()")
        {
            auto pipeline = int_to_int() >>= interceptor >>= interceptor >>= int_to_int() >>= int_to_string();
            AND_WHEN("pipeline is invoked")
            {
                std::string result = 1 >>= pipeline;
                THEN("the custom interceptor handles invocation of downstream callables")
                {
                    REQUIRE(result == "3");
                }
            }
        }
    }
}
SCENARIO("built in pipeline interceptors")
{
    GIVEN("a pipeline composed as: iterable >>= for_each >>= receiver")
    {
        std::vector<int> receivedValues;
        auto pipeline = for_each >>= [&](int val){ receivedValues.push_back(val); };
        
        WHEN("an iterable type is piped")
        {
            std::vector<int> sendValues{1, 2, 3};
            sendValues >>= pipeline;
            THEN("each individual value is forwarded to receiver")
            {
                REQUIRE(receivedValues.size() == 3);
                REQUIRE(receivedValues[0] == 1);
                REQUIRE(receivedValues[1] == 2);
                REQUIRE(receivedValues[2] == 3);
            }
        }
    }
    GIVEN("a pipeline composed as: variant<x, y, z>  >>=  visitor  >>=  receiver")
    {
        struct
        {
            int receivedInt = 0;
            float receivedFloat = 0.0f;
            std::string receivedStr = "";
            void operator()(int val) { receivedInt = val; }
            void operator()(float val) { receivedFloat = val; }
            void operator()(std::string val) { receivedStr = val; }
        } receiver;

        auto pipeline = visitor >>= &receiver;

        WHEN("a variant of multiple types is piped, once while containing each time")
        {
            std::variant<int, float, std::string> inputValues;
            inputValues = (int)1;
            inputValues >>= pipeline;
            inputValues = (float)2.0;
            inputValues >>= pipeline;
            inputValues = "Hello";
            inputValues >>= pipeline;

            THEN("each individual value is forwarded to receiver")
            {
                REQUIRE(receiver.receivedInt == 1);
                REQUIRE(receiver.receivedFloat == 2.0);
                REQUIRE(receiver.receivedStr == "Hello");
            }
        }
    }
    GIVEN("a pipeline composed as: optional<x>  >>=  maybe  >>=  receiver")
    {
        struct
        {
            bool wasCalled = false;
            int receivedInt = 0;
            void operator()(int val)
            {
                receivedInt = val;
                wasCalled = true;
            }
        } receiver;

        auto pipeline = maybe >>= &receiver;

        WHEN("an optional with no value is piped")
        {
            std::optional<int> noValue;
            noValue >>= pipeline;

            THEN("receiver is not invoked")
            {
                REQUIRE(receiver.wasCalled == false);
            }
        }
        WHEN("an optional with value is piped")
        {
            std::optional<int> value = 1;
            value >>= pipeline;

            THEN("receiver is invoked")
            {
                REQUIRE(receiver.wasCalled);
                REQUIRE(receiver.receivedInt == 1);
            }
        }
    }
}