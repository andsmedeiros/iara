/**
 * @file test/src/fuss/test.cpp
 * @brief 
 * @author André Medeiros
 * @date 08/08/23
 * @copyright 2023 (C) André Medeiros
**/

#include <catch2/catch_test_macros.hpp>
#include <fuss.hpp>
#include <utils/test-helpers.hpp>

using namespace std::string_literals;
using namespace utils::test_helpers;

SCENARIO("a shouter can be created for messages of different types", "[fuss]") {
    GIVEN("various messages of various types (msg_1, msg_2, msg_3)") {
        struct msg_1 : public fuss::message<> {  };
        struct msg_2 : public fuss::message<std::string> {  };
        struct msg_3 : public fuss::message<bool, int> {  };

        AND_GIVEN("a shouter type capable of shouting all these messages") {
            struct test_shouter : public fuss::shouter<msg_1, msg_2, msg_3> {  };

            WHEN("an instance of it is created") {
                auto create_result = attempt([&] {
                    [[maybe_unused]] test_shouter shouter;
                });

                THEN("an exception must not have been thrown") {
                    REQUIRE_FALSE(create_result.has_error());
                }
            }
        }
    }
}

SCENARIO("a shouter can be listened to and can shout", "[fuss]") {
    GIVEN("some message types and a shouter type") {
        struct msg_1 : public fuss::message<> {  };
        struct msg_2 : public fuss::message<std::string> {  };
        struct test_shouter : public fuss::shouter<msg_1, msg_2> {  };

        AND_GIVEN("a shouter instance") {
            test_shouter shouter;

            struct {
                int msg_2 = 0;
                int msg_1 = 0;
            } handler_count;

            AND_WHEN("its .listen<msg_1>() function is called with a functor") {
                auto listen_1_result = attempt([&] {
                    return shouter.listen<msg_1>([&] {
                        handler_count.msg_1++;
                    });
                });

                THEN("no exception must have been thrown") {
                    REQUIRE_FALSE(listen_1_result.has_error());
                }

                THEN("it must have returned a message listener") {
                    REQUIRE(listen_1_result.holds_value<fuss::listener>());

                    auto &listener = listen_1_result.get_value();

                    WHEN("the shouter's .shout<msg_1>() function is called") {
                        auto shout_result = attempt([&] {
                            shouter.shout<msg_1>();
                        });

                        THEN("no exception must have been thrown") {
                            REQUIRE_FALSE(shout_result.has_error());
                        }

                        THEN("the functor provided as msg_1's handler must have been invoked") {
                            REQUIRE(handler_count.msg_1 == 1);
                        }
                    }

                    WHEN("the listener's .cancel() function is called") {
                        auto cancel_result = attempt([&] {
                            listener.cancel();
                        });

                        THEN("no exception must have been thrown") {
                            REQUIRE_FALSE(cancel_result.has_error());
                        }

                        AND_WHEN("the shouter shouts msg_1") {
                            shouter.shout<msg_1>();

                            THEN("the functor provided as msg_1's handler must not have been invoked") {
                                REQUIRE(handler_count.msg_1 == 0);
                            }
                        }
                    }
                }

                AND_WHEN("the shouter's msg_2 is being listened") {
                    std::string shouted_string;
                    auto shout_2_listener_1 =
                        shouter.listen<msg_2>([&] (const auto &value) {
                            handler_count.msg_2++;
                            shouted_string = value;
                        });

                    AND_WHEN("the shouter shouts msg_2") {
                        shouter.shout<msg_2>("message 2 shouted"s);

                        THEN("the attached handler must have been executed") {
                            REQUIRE(handler_count.msg_2 == 1);
                            REQUIRE(shouted_string == "message 2 shouted"s);
                        }

                        THEN("any handlers attached to msg_1 must not have been executed") {
                            REQUIRE(handler_count.msg_1 == 0);
                        }
                    }

                    AND_WHEN("another handler is attached to msg_2") {
                        auto shouter_2_listener_2 =
                            shouter.listen<msg_2>([&] (auto) {
                                handler_count.msg_2++;
                            });

                        AND_WHEN("the shouter shouts msg_2") {
                            shouter.shout<msg_2>("message 2 shouted"s);

                            THEN("both handlers must have been executed") {
                                REQUIRE(handler_count.msg_2 == 2);
                                REQUIRE(shouted_string == "message 2 shouted"s);
                            }

                            THEN("any handlers attached to msg_1 must not have been executed") {
                                REQUIRE(handler_count.msg_1 == 0);
                            }
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("a message guard can manage the lifetime of a listener", "[fuss]") {
    GIVEN("a one-message shouter") {
        struct msg : public fuss::message<> {  };
        struct test_shouter : public fuss::shouter<msg> {  };

        test_shouter shouter;

        WHEN("it is listened to") {
            bool handler_executed = false;
            auto listener = shouter.listen<msg>([&] {
                handler_executed = true;
            });

            AND_WHEN("it shouts") {
                shouter.shout<msg>();

                THEN("the attached handler must have been executed") {
                    REQUIRE(handler_executed);
                }
            }

            AND_WHEN("the returned listener is yielded to a message guard") {
                auto guard_result = attempt([&] {
                    return new fuss::message_guard { std::move(listener) };
                });

                THEN("no exception must have been thrown") {
                    REQUIRE_FALSE(guard_result.has_error());

                    auto *guard = guard_result.get_value();

                    AND_WHEN("the message is shouted") {
                        shouter.shout<msg>();

                        THEN("the attached handler must have been executed") {
                            REQUIRE(handler_executed);
                        }
                    }

                    AND_WHEN("the message guard is destroyed") {
                        auto delete_result = attempt([&] {
                            delete guard;
                        });

                        THEN("no exception must have been thrown") {
                            REQUIRE_FALSE(delete_result.has_error());
                        }

                        AND_WHEN("the message is shouted") {
                            shouter.shout<msg>();

                            THEN("the attached handler must not have been executed") {
                                REQUIRE_FALSE(handler_executed);
                            }
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("shouter groups can aggregate multiple shouter inheritance chains", "[fuss]") {
    GIVEN("a shouter type") {
        struct msg_1 : public fuss::message<> {  };
        struct base_shouter : public fuss::shouter<msg_1> {  };

        AND_GIVEN("another message type") {
            struct msg_2 : public fuss::message<> {  };

            THEN("it is possible to create a group of shouters type") {
                struct group_shouter :
                        public fuss::group<base_shouter, fuss::shouter<msg_2>> {  };

                AND_WHEN("an instance is constructed") {
                    auto construct_result = attempt([] {
                        [[maybe_unused]] group_shouter shouter;
                    });

                    THEN("no exception must have been thrown") {
                        REQUIRE_FALSE(construct_result.has_error());
                    }
                }

                AND_GIVEN("an instance of it") {
                    group_shouter shouter;

                    THEN("it is possible to listen for both messages") {
                        struct {
                            bool msg_1 = false;
                            bool msg_2 = false;
                        } handler_executed;
                        shouter.listen<msg_1>([&] {
                            handler_executed.msg_1 = true;
                        });
                        shouter.listen<msg_2>([&] {
                            handler_executed.msg_2 = true;
                        });

                        AND_WHEN("the shouter shouts msg_1") {
                            shouter.shout<msg_1>();

                            THEN("the attached handler must have been executed") {
                                REQUIRE(handler_executed.msg_1);
                            }
                        }

                        AND_WHEN("the shouter shouts msg_2") {
                            shouter.shout<msg_2>();

                            THEN("the attached handler must have been executed") {
                                REQUIRE(handler_executed.msg_2);
                            }
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("exceptions thrown inside message handlers must propagate") {
    GIVEN("a shouter") {
        struct msg : public fuss::message<> {  };
        struct test_shouter : public fuss::shouter<msg> {  };

        test_shouter shouter;

        WHEN("it is attached a handler that throws an exception") {
            auto listen_result = attempt([&] {
                shouter.listen<msg>([] {
                    throw std::runtime_error { "handler exception"s };
                });
            });

            THEN("no exception must have been thrown") {
                REQUIRE_FALSE(listen_result.has_error());
            }

            AND_WHEN("the message is shouted") {
                auto shout_result = attempt([&] {
                    shouter.shout<msg>();
                });

                THEN("the exception from the handler must have been propagated") {
                    REQUIRE(shout_result.holds_error<std::runtime_error>());
                    REQUIRE(
                        shout_result.get_error<std::runtime_error>().what() ==
                        "handler exception"s
                    );
                }
            }
        }
    }
}