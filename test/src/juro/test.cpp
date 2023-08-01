#define JURO_TEST

#include <type_traits>
#include <string>
#include <catch2/catch_test_macros.hpp>
#include "juro/promise.hpp"
#include "juro/compose/all.hpp"
#include "juro/compose/race.hpp"
#include "test/juro/helpers.hpp"

using namespace juro::helpers;
using namespace juro::test::helpers;
using namespace std::string_literals;

SCENARIO("a promise can be created on every state", "[juro]") {
    GIVEN("a pending promise factory function") {
        WHEN("it is called with no parameter") {
            auto promise = juro::make_pending();

            THEN("it should create a pending `void` promise") {
                STATIC_REQUIRE(std::is_void_v<
                    typename decltype(promise)::element_type::type
                >);
                REQUIRE(promise->get_state() == promise_state::PENDING);
                REQUIRE(promise->is_pending());
                REQUIRE_FALSE(promise->is_settled());
                REQUIRE(promise->holds_value<empty_type>());
                REQUIRE_FALSE(promise->has_handler());
            }
        }
    }

    GIVEN("a resolved promise factory function") {
        WHEN("it is called with an `int` parameter") {
            auto promise = juro::make_resolved(100);

            THEN("it should create a resolved `int` promise") {
                STATIC_REQUIRE(std::is_same_v<
                    typename decltype(promise)::element_type::type, 
                    int
                >);
                REQUIRE(promise->get_state() == promise_state::RESOLVED);
                REQUIRE(promise->is_resolved());
                REQUIRE(promise->is_settled());
                REQUIRE(promise->holds_value<int>());
                REQUIRE_FALSE(promise->has_handler());
                REQUIRE_NOTHROW(promise->get_value());
                REQUIRE(promise->get_value() == 100);
            }
        }
    }

    GIVEN("a rejected promise factory function") {
        WHEN("it is called with a `std::string` parameter") {
            auto promise = juro::make_rejected<std::string>("Rejected promise"s);

            THEN("it should create a rejected `std::string` promise") {
                STATIC_REQUIRE(std::is_same_v<
                    typename decltype(promise)::element_type::type, 
                    std::string>
                );
                REQUIRE(promise->get_state() == promise_state::REJECTED);
                REQUIRE(promise->is_rejected());
                REQUIRE(promise->is_settled());
                REQUIRE(promise->holds_value<std::exception_ptr>());
                REQUIRE_FALSE(promise->has_handler());

                REQUIRE_NOTHROW(promise->get_error());
                auto result = rescue(promise->get_error());

                REQUIRE(result.has_error());
                REQUIRE(result.get_error<std::string>() == "Rejected promise"s);
            }
        }
    }
}

SCENARIO("promises should resolve and reject accordingly", "[juro]") {
    GIVEN("a pending promise") {
        auto promise = juro::make_pending<bool>();

        WHEN("resolve() is called") {
            auto result = attempt([&] { promise->resolve(true); });

            THEN("no exception must occur") {
                REQUIRE(result.has_value());

                AND_THEN("the promise should be resolved") {
                    REQUIRE(promise->is_resolved());
                    REQUIRE(promise->get_value());
                }
            }
        }

        WHEN("reject() is called") {
            auto result = attempt([&] { promise->reject("Rejected"s); });

            THEN("a `promise_error` must be thrown") {
                REQUIRE(result.has_error());
                REQUIRE(result.holds_error<promise_error>());
                REQUIRE(result.get_error<promise_error>().what() == 
                    "Unhandled promise rejection"s
                );

                AND_THEN("the promise should be rejected") {
                    REQUIRE(promise->is_rejected());
                    REQUIRE(promise->holds_value<std::exception_ptr>());
                    REQUIRE(rescue(promise->get_error())
                        .get_error<std::string>() == "Rejected"s
                    );
                }
            }
        }
    }
}

SCENARIO("promises must not be resettled", "[juro]") {
    GIVEN("a resolved promise") {
        auto promise = juro::make_resolved();

        WHEN("`resolve()` is called") {
            auto result = attempt([&] { promise->resolve(); });

            THEN("it must throw a `promise_error`") {
                REQUIRE(result.has_error());
                REQUIRE(result.holds_error<promise_error>());
                REQUIRE(result.get_error<promise_error>().what() == 
                    "Attempted to resolve an already settled promise"s
                );
            }
        }

        WHEN("`reject()` is called") {
            auto result = attempt([&] { promise->reject(); });

            THEN("it must throw a `promise_error`") {
                REQUIRE(result.has_error());
                REQUIRE(result.holds_error<promise_error>());
                REQUIRE(result.get_error<promise_error>().what() == 
                    "Attempted to reject an already settled promise"s
                );
            }
        }
    }

    GIVEN("a rejected promise") {
        auto promise = juro::make_rejected();

        WHEN("`resolve()` is called") {
            auto result = attempt([&] { promise->resolve(); });

            THEN("it must throw a `promise_error`") {
                REQUIRE(result.has_error());
                REQUIRE(result.holds_error<promise_error>());
                REQUIRE(result.get_error<promise_error>().what() == 
                    "Attempted to resolve an already settled promise"s
                );
            }
        }

        WHEN("`reject()` is called") {
            auto result = attempt([&] { promise->reject(); });

            THEN("it must throw a `promise_error`") {
                REQUIRE(result.has_error());
                REQUIRE(result.holds_error<promise_error>());
                REQUIRE(result.get_error<promise_error>().what() == 
                    "Attempted to reject an already settled promise"s
                );
            }
        }
    }
}

SCENARIO("promises should be chainable", "[juro]") {
    GIVEN("a pending promise") {
        auto promise = juro::make_pending<int>();

        WHEN("`then` is called with one argument") {
            auto next = promise->then([] (int value) { return value; });

            THEN("a settle handler must be attached") {
                REQUIRE(promise->has_handler());
            }

            AND_WHEN("the promise is resolved with a value") {
                auto result = attempt([&] { promise->resolve(10); });

                THEN("no exception should be thrown") {
                    REQUIRE(result.has_value());

                    AND_THEN("the chained promise must be resolved with the same value") {
                        REQUIRE(next->is_resolved());
                        REQUIRE(next->get_value() == 10);
                    }
                }
            }

            AND_WHEN("the promise is rejected with a value") {
                auto result = attempt([&] { promise->reject("Rejected"s); });

                THEN("a `promise_error` must be thrown") {
                    REQUIRE(result.has_error());
                    REQUIRE(result.holds_error<promise_error>());
                    REQUIRE(result.get_error<promise_error>().what() == 
                        "Unhandled promise rejection"s
                    );

                    AND_THEN("the chained promise must be rejected with 'Rejected'") {
                        REQUIRE(next->is_rejected());
                        REQUIRE(next->holds_value<std::exception_ptr>());
                        REQUIRE(rescue(next->get_error())
                            .get_error<std::string>() == "Rejected"
                        );
                    }
                }
            }
        }

        WHEN("`finally()` is called") {
            bool handled = false;
            finally_argument_t<int> value;
            auto next = promise->finally([&] (auto &result) { 
                handled = true; 
                value = result;
                return "Resolved"s;
            });

            THEN("a settle handler must be attached") {
                REQUIRE(promise->has_handler());
            }

            AND_WHEN("the promise is resolved with a value") {
                auto result = attempt([&] { promise->resolve(-100); });

                THEN("no exception should be thrown") {
                    REQUIRE(result.has_value());

                    AND_THEN("the chained promise must be resolved with 'Resolved'") {
                        REQUIRE(next->is_resolved());
                        REQUIRE(next->holds_value<std::string>());
                        REQUIRE(next->get_value() == "Resolved"s);

                        AND_THEN("the finally handler must have been called") {
                            REQUIRE(handled);
                            REQUIRE(std::holds_alternative<int>(value));
                            REQUIRE(std::get<int>(value) == -100);
                        }
                    }
                }
            }

            AND_WHEN("the promise is rejected with a value") {
                auto result = attempt([&] { promise->reject("Rejected"s); });

                THEN("no exception should be thrown") {
                    REQUIRE(result.has_value());

                    AND_THEN("the chained promise must be resolved") {
                        REQUIRE(next->is_resolved());
                        REQUIRE(next->holds_value<std::string>());
                        REQUIRE(next->get_value() == "Resolved"s);

                        AND_THEN("the finally handler must have been called") {
                            REQUIRE(handled);
                            REQUIRE(std::holds_alternative<std::exception_ptr>(value));
                            REQUIRE(
                                rescue(std::get<std::exception_ptr>(value))
                                    .holds_error<std::string>()
                            );
                            REQUIRE(
                                rescue(std::get<std::exception_ptr>(value))
                                    .get_error<std::string>() == "Rejected"s
                            );
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("promises should be composable", "[juro]") {
    GIVEN("a promise composition function `all()`") {
        WHEN("called with three promises of different types") {
            auto p1 = juro::make_pending<int>();
            auto p2 = juro::make_pending<std::string>();
            auto p3 = juro::make_pending();

            auto all_result = attempt([&] {
                return juro::all(p1, p2, p3);
            });

            THEN("it must not throw an exception") {
                REQUIRE_FALSE(all_result.has_error());

                AND_THEN("it must return a pending promise") {
                    REQUIRE(all_result.has_value());
                    REQUIRE(all_result.get_value()->is_pending());
                }
            }

            AND_WHEN("some promises are resolved") {
                auto &promise = all_result.get_value();
                p1->resolve(10);
                p3->resolve();

                THEN("the returned promise must still be pending") {
                    REQUIRE(promise->is_pending());
                }

                AND_WHEN("all promises are resolved") {
                    p2->resolve("resolved");

                    THEN("the returned promise must be resolved with correct values") {
                        REQUIRE(promise->is_resolved());
                        REQUIRE(promise->get_value() == 
                            std::make_tuple(10, "resolved"s, void_type { })
                        );
                    }
                }
            }

            AND_WHEN("any promise is rejected") {
                auto p2_result = attempt([&] { p2->reject("Rejected"s); });
                auto &promise = all_result.get_value();

                THEN("a `promise_error` must be thrown") {
                    REQUIRE(p2_result.has_error());
                    REQUIRE(p2_result.holds_error<promise_error>());
                    REQUIRE(p2_result.get_error<promise_error>().what() ==
                        "Unhandled promise rejection"s
                    );

                    AND_THEN("the returned promise must be rejected with 'Rejected'") {
                        REQUIRE(promise->is_rejected());
                        REQUIRE(rescue(promise->get_error())
                            .get_error<std::string>() == "Rejected"s
                        );
                    }
                }

                AND_WHEN("other promises are resolved") {
                    auto p1_result = attempt([&] { p1->resolve(0); });

                    THEN("no exception should be thrown") {
                        REQUIRE(p1_result.has_value());

                        AND_THEN("the returned promise must retain its value") {
                            REQUIRE(promise->is_rejected());
                            REQUIRE(rescue(promise->get_error())
                                .get_error<std::string>() == "Rejected"s
                            );
                        }
                    }
                }

                AND_WHEN("other promises are rejected") {
                    auto p3_result = attempt([&] { p3->reject("Invalid"s); });

                    THEN("no exception should be thrown") {
                        REQUIRE(p3_result.has_value());

                        AND_THEN("the returned promise must retain its value") {
                            REQUIRE(promise->is_rejected());
                            REQUIRE(rescue(promise->get_error())
                                .get_error<std::string>() == "Rejected"s
                            );
                        }
                    }
                }
            }
        }

        WHEN("called with only void promises") {
            auto p1 = juro::make_pending();
            auto p2 = juro::make_pending();
            auto p3 = juro::make_pending();

            auto all_result = attempt([&] {
                return juro::all(p1, p2, p3);
            });

            THEN("it must not throw an exception") {
                REQUIRE_FALSE(all_result.has_error());

                AND_THEN("the type of the returned promise must also be void") {
                    STATIC_REQUIRE(
                        std::is_same_v<decltype(all_result)::value_type, juro::promise_ptr<void>>
                    );
                }
            }
        }

    }

    GIVEN("a promise composition function `race()`") {
        WHEN("called with three promises of different types") {
            auto p1 = juro::make_pending<int>();
            auto p2 = juro::make_pending<std::string>();
            auto p3 = juro::make_pending();

            auto race_result = attempt([&] { return juro::race(p1, p2, p3); });

            THEN("it must not throw an exception") {
                REQUIRE_FALSE(race_result.has_error());

                AND_THEN("it must return a pending promise") {
                    REQUIRE(race_result.has_value());
                    REQUIRE(race_result.get_value()->is_pending());
                }
            }

            AND_WHEN("a promise is resolved with some value") {
                auto &promise = race_result.get_value();
                p2->resolve("Resolved"s);

                THEN("the returned promise must be resolved with the same value") {
                    REQUIRE(promise->is_resolved());
                    REQUIRE(std::holds_alternative<std::string>(promise->get_value()));
                    REQUIRE(std::get<std::string>(promise->get_value()) == "Resolved"s);
                }

                AND_WHEN("another promise is resolved") {
                    auto p3_result = attempt([&] { p3->resolve(); });

                    THEN("no exception must be thrown") {
                        REQUIRE_FALSE(p3_result.has_error());

                        AND_THEN("the returned promise must retain its value") {
                            REQUIRE(promise->is_resolved());
                            REQUIRE(std::holds_alternative<std::string>(promise->get_value()));
                            REQUIRE(std::get<std::string>(promise->get_value()) == 
                                "Resolved"s
                            );
                        }
                    }

                    AND_WHEN("the last promise is rejected") {
                        auto p1_result = attempt([&] { p1->reject(100); });

                        THEN("no exception must be thrown") {
                            REQUIRE_FALSE(p1_result.has_error());

                            AND_THEN("the returned promise must retain its value") {
                                REQUIRE(promise->is_resolved());
                                REQUIRE(std::holds_alternative<std::string>(promise->get_value()));
                                REQUIRE(std::get<std::string>(promise->get_value()) == 
                                    "Resolved"s
                                );
                            }
                        }
                    }
                }
            }

            AND_WHEN("a promise is rejected with some value") {
                auto &promise = race_result.get_value();
                auto p2_result = attempt([&] { p2->reject("Rejected"s); });

                THEN("a `promise_error` exception must be thrown") {
                    REQUIRE(p2_result.has_error());
                    REQUIRE(p2_result.holds_error<promise_error>());
                    REQUIRE(p2_result.get_error<promise_error>().what() == 
                        "Unhandled promise rejection"s
                    );
                }

                THEN("the returned promise must be rejected with the same value") {
                    REQUIRE(promise->is_rejected());
                    REQUIRE(rescue(promise->get_error()).holds_error<std::string>());
                    REQUIRE(rescue(promise->get_error())
                        .get_error<std::string>() == "Rejected"s
                    );
                }

                AND_WHEN("another promise is resolved") {
                    auto p3_result = attempt([&] { p3->resolve(); });

                    THEN("no exception must be thrown") {
                        REQUIRE_FALSE(p3_result.has_error());

                        AND_THEN("the returned promise must retain its value") {
                            REQUIRE(promise->is_rejected());
                            REQUIRE(rescue(promise->get_error()).holds_error<std::string>());
                            REQUIRE(rescue(promise->get_error())
                                .get_error<std::string>() == "Rejected"s
                            );
                        }
                    }

                    AND_WHEN("the last promise is rejected") {
                        auto p1_result = attempt([&] { p1->reject(100); });

                        THEN("no exception must be thrown") {
                            REQUIRE_FALSE(p1_result.has_error());

                            AND_THEN("the returned promise must retain its value") {
                                REQUIRE(promise->is_rejected());
                                REQUIRE(rescue(promise->get_error()).holds_error<std::string>());
                                REQUIRE(rescue(promise->get_error())
                                    .get_error<std::string>() == "Rejected"s
                                );
                            }
                        }
                    }
                }
            }
        }
    }
}