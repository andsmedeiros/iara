/**
 * @file fugax/test/src/test.cpp
 * @brief Fugax test routines
 * @author André Medeiros
 * @date 27/06/23
 * @copyright 2023 (C) André Medeiros
**/


#include <catch2/catch_test_macros.hpp>
#include <fugax/event-loop.hpp>

#include "test/fugax/helpers.hpp"

using namespace fugax::test::helpers;
using namespace std::string_literals;
using namespace utils::types;

SCENARIO("an event loop can be created", "[fugax]") {
    GIVEN("an event loop constructor") {
        WHEN("it is invoked") {
            auto result = attempt([] {
                [[maybe_unused]] fugax::event_loop loop;
            });

            THEN("it should not throw anything") {
                REQUIRE_FALSE(result.has_error());
            }
        }
    }
}

SCENARIO("an event loop can be operated accordingly", "[fugax]") {
    GIVEN("an event loop") {
        fugax::event_loop loop;

        WHEN("a task is scheduled for immediate execution") {
            bool task_executed = false;

            schedule_for_test([&] {
                return loop.schedule([&] { task_executed = true; });
            }, [&] (auto &listener) {

                THEN("the task must not have been executed") {
                    REQUIRE_FALSE(task_executed);
                }

                AND_WHEN("the event loop is stimulated to process scheduled events") {
                    auto process_result = attempt([&] {
                        loop.process(0);
                    });

                    THEN("no exception should have been thrown") {
                        REQUIRE_FALSE(process_result.has_error());
                    }

                    THEN("the task must have been executed") {
                        REQUIRE(task_executed);
                    }

                    THEN("the scheduled event must have been destroyed") {
                        REQUIRE(listener.expired());
                    }
                }
            });
        }

        WHEN("a task is scheduled for future execution") {
            bool task_executed = false;

            schedule_for_test([&] {
                return loop.schedule(100, [&] { task_executed = true; });
            }, [&] (auto &listener) {

                THEN("the task must not have been executed") {
                    REQUIRE_FALSE(task_executed);
                }

                AND_WHEN(
                    "the event loop is stimulated with an updated time value, "\
                    "smaller than the task delay"
                ) {
                    loop.process(90);

                    THEN("the task must not have been executed") {
                        REQUIRE_FALSE(task_executed);
                    }

                    THEN("the scheduled event must still exist") {
                        REQUIRE_FALSE(listener.expired());
                    }
                }

                AND_WHEN(
                    "the event loop is stimulated with an updated time value, "\
                    "greater than the task delay"
                ) {
                    loop.process(110);

                    THEN("the task must have been executed") {
                        REQUIRE(task_executed);
                    }

                    THEN("the scheduled event must have been destroyed") {
                        REQUIRE(listener.expired());
                    }
                }

                AND_WHEN("the task is rescheduled to a further time value") {
                    auto reschedule_result = attempt([&] {
                        listener.lock()->reschedule(200);
                    });

                    THEN("not exception must have been thrown") {
                        REQUIRE_FALSE(reschedule_result.has_error());
                    }

                    AND_WHEN(
                        "the event loop is stimulated with an updated time value, "\
                        "greater than the initial task delay and smaller than the "\
                        "rescheduled delay"
                    ) {
                        loop.process(110);

                        THEN("the task must not have been executed") {
                            REQUIRE_FALSE(task_executed);
                        }

                        THEN("the event must still exist") {
                            REQUIRE_FALSE(listener.expired());
                        }

                        AND_WHEN(
                            "the event loop is stimulated with an updated time value, "\
                            "greater than the rescheduled delay"
                        ) {
                            loop.process(210);

                            THEN("the task must have been executed") {
                                REQUIRE(task_executed);
                            }

                            THEN("the event must have been destroyed") {
                                REQUIRE(listener.expired());
                            }
                        }
                    }
                }

                AND_WHEN("the task is cancelled") {
                    auto cancel_result = attempt([&] {
                        listener.lock()->cancel();
                    });

                    THEN("no exception must have been thrown") {
                        REQUIRE_FALSE(cancel_result.has_error());
                    }

                    AND_WHEN(
                        "the event loop is stimulated with an updated time value, "\
                            "greater than the task delay"
                    ) {
                        loop.process(110);

                        THEN("the task must still not have been executed") {
                            REQUIRE_FALSE(task_executed);
                        }

                        THEN("the scheduled event must have been destroyed") {
                            REQUIRE(listener.expired());
                        }
                    }
                }
            });
        }

        WHEN("a task is scheduled for recurring execution") {
            const auto interval = 10;
            int execution_count = 0;
            schedule_for_test([&] {
                return loop.schedule(interval, true,[&] { execution_count++; });
            }, [&] (auto &listener) {

                THEN("the task must not have been executed") {
                    REQUIRE(execution_count == 0);
                }

                AND_WHEN("the task interval is elapsed") {
                    loop.process(interval);

                    THEN("the task must have been executed once") {
                        REQUIRE(execution_count == 1);

                        AND_THEN("the event must still exist") {
                            REQUIRE_FALSE(listener.expired());
                        }
                    }

                    AND_WHEN("the task interval is elapsed again") {
                        loop.process(2 * interval);

                        THEN("the task must have been executed twice") {
                            REQUIRE(execution_count == 2);

                            AND_THEN("the event must still exist") {
                                REQUIRE_FALSE(listener.expired());
                            }
                        }
                    }
                }
            });
        }

        WHEN("a task is scheduled for immediate recurring execution") {
            const auto interval = 10;
            int execution_count = 0;
            schedule_for_test([&] {
                return loop.schedule(
                    interval,
                    fugax::schedule_policy::recurring_immediate,
                    [&] { execution_count++; }
                );
            }, [&] (auto &listener) {

                THEN("the task must not have been executed") {
                    REQUIRE(execution_count == 0);
                }

                AND_WHEN("the event loop is stimulated to process immediate events") {
                    loop.process(0);

                    THEN("the task must have been executed once") {
                        REQUIRE(execution_count == 1);

                        AND_THEN("the event must still exist") {
                            REQUIRE_FALSE(listener.expired());
                        }
                    }

                    AND_WHEN("the task interval is elapsed") {
                        loop.process(interval);

                        THEN("the task must have been executed twice") {
                            REQUIRE(execution_count == 2);

                            AND_THEN("the event must still exist") {
                                REQUIRE_FALSE(listener.expired());
                            }
                        }

                        AND_WHEN("the task interval is elapsed again") {
                            loop.process(2 * interval);

                            THEN("the task must have been executed three times") {
                                REQUIRE(execution_count == 3);

                                AND_THEN("the event must still exist") {
                                    REQUIRE_FALSE(listener.expired());
                                }
                            }
                        }
                    }
                }
            });
        }

        WHEN("a task is scheduled for continuous execution") {
            int execution_count = 0;
            fugax::time_type clock = 0;

            schedule_for_test([&] {
                return loop.always([&] { execution_count++; });
            }, [&] (auto &listener) {
                THEN("the task must not have been executed") {
                    REQUIRE(execution_count == 0);
                }

                AND_WHEN("the event loop is stimulated with the current time value") {
                    loop.process(clock);

                    THEN("the task must have been executed once") {
                        REQUIRE(execution_count == 1);

                        AND_WHEN("the event loop is stimulated with the same time value once more") {
                            loop.process(clock);

                            THEN("the task must have been executed twice") {
                                REQUIRE(execution_count == 2);
                            }

                            AND_WHEN("the event loop is stimulated with a very different time value") {
                                clock += 100;
                                loop.process(clock);

                                THEN("the task must have been executed three times") {
                                    REQUIRE(execution_count == 3);

                                    AND_THEN("the event must still exist") {
                                        REQUIRE_FALSE(listener.expired());
                                    }
                                }
                            }
                        }
                    }
                }
            });
        }

        WHEN("its .wait() function is called with some time delay") {
            auto wait_result = attempt([&] {
                return loop.wait(100);
            });

            THEN("no exception must have been thrown") {
                REQUIRE_FALSE(wait_result.has_error());
            }

            THEN("a pending juro::promise_ptr<fugax::timeout> must have been returned") {
                REQUIRE(wait_result.holds_value<juro::promise_ptr<fugax::timeout>>());

                auto promise = wait_result.get_value();
                REQUIRE(static_cast<bool>(promise));
                REQUIRE(promise->is_pending());

                AND_WHEN("a resolve handler is attached to the promise and the delay is elapsed") {
                    bool promise_resolved = false;
                    promise->then([&] (auto) {
                        promise_resolved = true;
                    });

                    loop.process(110);

                    THEN("the promise must have been resolved") {
                        REQUIRE(promise_resolved);
                    }
                }
            }
        }

        WHEN("its .timeout() function is called with a promise as parameter") {
            auto promise = juro::make_pending<std::string>();
            auto timeout_result = attempt([&] {
                return loop.timeout(100, promise);
            });

            THEN("no exception must have been thrown") {
                REQUIRE_FALSE(timeout_result.has_error());
            }

            THEN("it must have returned a promise pointer to a timeout variant type") {
                REQUIRE(timeout_result.holds_value<
                    juro::promise_ptr<std::variant<std::string, fugax::timeout>>
                >());

                auto &timeout_promise = timeout_result.get_value();

                AND_WHEN("settle handlers are attached to the timeout promise") {
                    std::variant<std::string, fugax::timeout> resolved_value;
                    std::exception_ptr rejected_value;

                    timeout_promise->then(
                        [&] (auto &result) {
                            resolved_value = result;
                        },
                        [&] (auto &error) {
                            rejected_value = error;
                        }
                    );

                    AND_WHEN("the initial promise is resolved") {
                        promise->resolve("resolved"s);

                        THEN("the timeout promise must also have been resolved with the same value") {
                            REQUIRE(timeout_promise->is_resolved());
                            REQUIRE(std::holds_alternative<std::string>(resolved_value));
                            REQUIRE(std::get<std::string>(resolved_value) == "resolved"s);
                        }

                    }

                    AND_WHEN("the initial promise is rejected") {
                        promise->reject("rejected"s);

                        THEN("the timeout promise must also have been rejected with the same value") {
                            REQUIRE(timeout_promise->is_rejected());
                            REQUIRE(rescue(rejected_value).holds_error<std::string>());
                            REQUIRE(rescue(rejected_value).get_error<std::string>() == "rejected"s);
                        }
                    }

                    AND_WHEN("the timeout is reached") {
                        loop.process(100);

                        THEN("the timeout promise must have been resolved with a fugax::timeout value") {
                            REQUIRE(timeout_promise->is_resolved());
                            REQUIRE(std::holds_alternative<fugax::timeout>(resolved_value));
                        }
                    }
                }
            }
        }

        WHEN("its .timeout() function is called with a promise launcher as parameter") {
            juro::promise_ptr<std::string> promise;
            auto timeout_result = attempt([&] {
                return loop.timeout<std::string>(100, [&] (auto &new_promise) {
                    promise = new_promise;
                });
            });

            THEN("no exception must have been thrown") {
                REQUIRE_FALSE(timeout_result.has_error());
            }

            THEN("it must have returned a promise pointer to a timeout variant type") {
                REQUIRE(timeout_result.holds_value<
                    juro::promise_ptr<std::variant<std::string, fugax::timeout>>
                >());

                auto &timeout_promise = timeout_result.get_value();

                AND_WHEN("settle handlers are attached to the timeout promise") {
                    std::variant<std::string, fugax::timeout> resolved_value;
                    std::exception_ptr rejected_value;

                    timeout_promise->then(
                        [&] (auto &result) {
                            resolved_value = result;
                        },
                        [&] (auto &error) {
                            rejected_value = error;
                        }
                    );

                    AND_WHEN("the initial promise is resolved") {
                        promise->resolve("resolved"s);

                        THEN("the timeout promise must also have been resolved with the same value") {
                            REQUIRE(timeout_promise->is_resolved());
                            REQUIRE(std::holds_alternative<std::string>(resolved_value));
                            REQUIRE(std::get<std::string>(resolved_value) == "resolved"s);
                        }

                    }

                    AND_WHEN("the initial promise is rejected") {
                        promise->reject("rejected"s);

                        THEN("the timeout promise must also have been rejected with the same value") {
                            REQUIRE(timeout_promise->is_rejected());
                            REQUIRE(rescue(rejected_value).holds_error<std::string>());
                            REQUIRE(rescue(rejected_value).get_error<std::string>() == "rejected"s);
                        }
                    }

                    AND_WHEN("the timeout is reached") {
                        loop.process(100);

                        THEN("the timeout promise must have been resolved with a fugax::timeout value") {
                            REQUIRE(timeout_promise->is_resolved());
                            REQUIRE(std::holds_alternative<fugax::timeout>(resolved_value));
                        }
                    }
                }
            }

        }
    }
}

SCENARIO("an event guard can be default-constructed", "[fugax]") {
    GIVEN("the default event guard constructor") {
        WHEN("it is invoked") {
            auto result = attempt([] {
               [[maybe_unused]] fugax::event_guard guard {  };
            });

            THEN("no exceptions must have been raised") {
                REQUIRE_FALSE(result.has_error());
            }
        }
    }
}

SCENARIO("an event guard can manage the lifetime of a scheduled event", "[fugax]") {
    GIVEN("an event loop") {
        fugax::event_loop loop;

        AND_GIVEN("a listener from a scheduled event") {
            bool task_executed = false;
            auto listener = loop.schedule(100, [&] {
                task_executed = true;
            });

            WHEN("an event guard is constructed from the listener") {
                auto guard_result = attempt([&] {
                    [[maybe_unused]] fugax::event_guard guard { listener };
                });

                THEN("no exception must have been thrown") {
                    REQUIRE_FALSE(guard_result.has_error());
                }
            }

            AND_GIVEN("an event guard managing the event") {
                auto *guard = new fugax::event_guard { listener };

                THEN("the event must still be scheduled") {
                    REQUIRE_FALSE(task_executed);
                    REQUIRE_FALSE(listener.expired());
                    REQUIRE_FALSE(listener.lock()->is_cancelled());
                }

                WHEN("the guard is destroyed") {
                    auto delete_result = attempt([&] {
                        delete guard;
                    });

                    THEN("no exception must have been thrown") {
                        REQUIRE_FALSE(delete_result.has_error());
                    }

                    THEN("the event must still exist") {
                        REQUIRE_FALSE(listener.expired());
                    }

                    THEN("the event must have been cancelled") {
                        REQUIRE(listener.lock()->is_cancelled());
                    }

                    AND_WHEN("the scheduled time arrives") {
                        loop.process(110);

                        THEN("the task must not have been executed") {
                            REQUIRE_FALSE(task_executed);
                        }

                        THEN("the event must have been destroyed") {
                            REQUIRE(listener.expired());
                        }
                    }
                }
            }
        }

        AND_GIVEN("two event guards managing two scheduled events") {
            bool task_1_executed = false, task_2_executed = false;
            fugax::event_guard guard_1 =
                loop.schedule(100, [&] {
                    task_1_executed = true;
                });
            fugax::event_guard guard_2 =
                loop.schedule(100, [&] {
                    task_2_executed = true;
                });

            THEN("both events must be still scheduled") {
                REQUIRE_FALSE(guard_1.get().expired());
                REQUIRE_FALSE(guard_2.get().expired());
            }

            WHEN("guard_2 is move-assigned to guard_1") {
                auto assignment_result = attempt([&] {
                    guard_1 = std::move(guard_2);
                });

                THEN("no exception must have been thrown") {
                    REQUIRE_FALSE(assignment_result.has_error());
                }

                THEN("guard_2 must be empty") {
                    REQUIRE(guard_2.get().expired());
                }

                THEN("guard_1 must be still scheduled") {
                    REQUIRE_FALSE(guard_1.get().expired());
                }

                AND_WHEN("the due time of both events arrive") {
                    loop.process(100);

                    THEN("only task_2 must have been executed") {
                        REQUIRE_FALSE(task_1_executed);
                        REQUIRE(task_2_executed);
                    }
                }
            }
        }
    }
}

SCENARIO("an event loop can debounce calls to a functor", "[fugax]") {
    GIVEN("an event loop") {
        fugax::event_loop loop;

        WHEN("its .debounce() function is called with a functor and some delay as parameters") {
            int counter = 0;
            auto functor = [&] { counter++; };

            auto debounce_result = attempt([&] {
                return loop.debounce(100, functor);
            });

            THEN("no exception must have been thrown") {
                REQUIRE_FALSE(debounce_result.has_error());
            }

            THEN("it must return a debounced functor with the same signature") {
                using result_type = decltype(debounce_result);
                STATIC_REQUIRE(std::is_invocable_r_v<void, result_type::value_type>);

                auto &debounced = debounce_result.get_value();

                AND_WHEN("the debounced functor is called") {
                    debounced();

                    THEN("the functor must not have been called") {
                        REQUIRE(counter == 0);
                    }

                    AND_WHEN("more time than the debounce delay has passed") {
                        test_clock clock;
                        loop.process(clock.advance(101));

                        THEN("the functor must have been called") {
                            REQUIRE(counter == 1);
                        }
                    }
                }

                AND_WHEN(
                    "the debounced functor is called multiple times, "\
                    "over a time span smaller than the debounce delay"
                ) {
                    test_clock clock;

                    for(int i = 0; i < 9; i++) {
                        debounced();
                        loop.process(clock.advance(10));
                    }

                    THEN("the functor must not have been called") {
                        REQUIRE(counter == 0);
                    }
                }

                AND_WHEN(
                    "the debounced functor is called multiple times, "\
                    "with an interval smaller than the debounce delay"
                ) {
                    test_clock clock;

                    for(int i = 0; i < 9; i++) {
                        debounced();
                        loop.process(clock.advance(99));
                    }

                    THEN("the functor must not have been called") {
                        REQUIRE(counter == 0);
                    }
                }

                AND_WHEN(
                    "the debounced functor is called multiple times, "\
                    "with an interval greater than the debounce delay"
                ) {
                    test_clock clock;

                    for(int i = 0; i < 9; i++) {
                        debounced();
                        loop.process(clock.advance(101));
                    }

                    THEN("the functor must have been executed that many times also") {
                        REQUIRE(counter == 9);
                    }
                }
            }
        }
    }
}

SCENARIO("an event loop can throttle calls to a functor", "[fugax]") {
    GIVEN("an event loop") {
        fugax::event_loop loop;

        WHEN("its .throttle() function is called with a functor and some delay as parameters") {
            int counter = 0;
            auto functor = [&] { counter++; };

            auto throttle_result = attempt([&] {
                return loop.throttle(100, functor);
            });

            THEN("no exception must have been thrown") {
                REQUIRE_FALSE(throttle_result.has_error());
            }

            THEN("it must return a functor with the same signature") {
                using result_type = decltype(throttle_result);
                STATIC_REQUIRE(std::is_invocable_r_v<void, result_type::value_type>);

                auto &throttled = throttle_result.get_value();

                AND_WHEN("the throttled functor is invoked") {
                    throttled();

                    THEN("the functor must have been executed") {
                        REQUIRE(counter == 1);
                    }

                    AND_WHEN("the throttled functor is invoked again") {
                        throttled();

                        THEN("the functor must not have been executed this time") {
                            REQUIRE(counter == 1);
                        }
                    }

                    AND_WHEN(
                        "the throttled functor is invoked multiple times, "\
                        "over a time span smalled than the throttle delay"
                    ) {
                        test_clock clock;

                        for(int i = 0; i < 9; i++) {
                            throttled();
                            loop.process(clock.advance(10));
                        }

                        THEN("the functor must not have been executed any more times") {
                            REQUIRE(counter == 1);
                        }
                    }

                    AND_WHEN(
                        "the throttled functor is invoked multiple times, "\
                        "with an interval smaller than the throttle delay"
                    ) {
                        test_clock clock;

                        fugax::time_type last = 0;
                        int expected = 1;

                        for(int i = 0; i < 9; i++) {
                            throttled();
                            loop.process(clock.advance(99));

                            if(clock - last > 100) {
                                expected++;
                                last = clock;
                            }
                        }

                        THEN(
                            "the functor must have been executed only so often as the "\
                            "delay had passed repeatedly"
                        ) {
                            REQUIRE(counter == expected);
                        }
                    }

                    AND_WHEN(
                        "the throttled functor is invoked multiple times, "\
                        "with an interval greater than the throttle delay"
                    ) {
                        test_clock clock;

                        for(int i = 0; i < 9; i++) {
                            throttled();
                            loop.process(clock.advance(101));
                        }

                        THEN("the functor must have been executed that many times") {
                            REQUIRE(counter == 9);
                        }
                    }
                }
            }
        }
    }
}