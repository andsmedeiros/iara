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
    }
}