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
                    [[maybe_unused]] test_shouter _ {  };
                });

                THEN("an exception must not have been thrown") {
                    REQUIRE_FALSE(create_result.has_error());
                }
            }

            AND_GIVEN("an instance of it") {
                test_shouter shouter;

                struct {
                    bool msg_1 = false;
                    bool msg_2 = false;
                    bool msg_3 = false;
                } handler_executed;

                AND_WHEN("its .listen<msg_1>() function is called with a functor") {
                    auto listen_1_result = attempt([&] {
                        return shouter.listen<msg_1>([&] {
                            handler_executed.msg_1 = true;
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
                                REQUIRE(handler_executed.msg_1);
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

                                THEN("the functor provied as msg_1's handler must not have been invoked") {
                                    REQUIRE_FALSE(handler_executed.msg_1);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}