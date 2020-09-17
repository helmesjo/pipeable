#include <pipeable/guarded_data_generator.hpp>

#include <catch2/catch.hpp>
#include <atomic>
#include <chrono>
#include <thread>

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

SCENARIO("Thread safe data generator")
{
    GIVEN("a thread safe data generator")
    {
        guarded_data_generator<int> generator; // Change this to data_generator<...> and it will crash as expected
        WHEN("one thread is continuously generating data")
        {
            std::atomic_bool start = false;
            const auto waitForStart = [&] {
                while (!start) { std::this_thread::sleep_for(std::chrono::milliseconds{ 1 }); }
            };

            const auto sendCount = 100;
            auto sendingThread = std::thread([&] {
                waitForStart();

                for (auto i = 0; i < sendCount; ++i)
                {
                    generator(1);
                }
            });
            
            AND_WHEN("multiple threads are continuously adding and removing receivers")
            {
                std::vector<std::thread> threads;
                const auto threadCount = 10;
                const auto registerCount = 100;
                for (auto i = 0; i < threadCount; ++i)
                {
                    auto thread = std::thread([&] {
                        waitForStart();

                        for (auto j = 0; j < registerCount; ++j)
                        {
                            int_to_int receiver;
                            generator += &receiver;
                            generator -= &receiver;
                        }
                    });
                    threads.push_back(std::move(thread));
                }

                THEN("it doesn't crash")
                {
                    start = true;
                }
                for (auto& thread : threads)
                {
                    thread.join();
                }
            }
            sendingThread.join();
        }
    }
}