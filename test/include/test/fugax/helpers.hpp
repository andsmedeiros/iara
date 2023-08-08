/**
 * @file test/include/test/fugax/helpers.hpp
 * @brief Helper functions and structures for testing purposes
 * @author André Medeiros
 * @date 31/07/23
 * @copyright 2023 (C) André Medeiros
**/

#ifndef FUGAX_TEST_HELPERS_HPP
#define FUGAX_TEST_HELPERS_HPP

#include "catch2/catch_test_macros.hpp"
#include "fugax/event-loop.hpp"
#include "utils/test-helpers.hpp"

namespace fugax::test::helpers {

using namespace utils::test_helpers;

template<class T_launcher, class T_then>
void schedule_for_test(T_launcher &&launcher, T_then &&then) {
    static_assert(std::is_invocable_v<T_launcher>, "Launcher must be invocable");
    auto result = attempt(launcher);

    THEN("no exception must have been thrown") {
        REQUIRE_FALSE(result.has_error());
    }

    THEN("a valid event listener must be returned") {
        REQUIRE(result.template holds_value<fugax::event_listener>());

        auto &listener = result.get_value();
        REQUIRE_FALSE(listener.expired());

        then(listener);
    }
}

struct test_clock {
    using time_type = fugax::time_type;
    time_type counter = 0;

    inline test_clock &advance(time_type by) noexcept {
        counter += by;
        return *this;
    }

    inline test_clock &regress(time_type by) noexcept {
        counter -= by;
        return *this;
    }

    constexpr operator time_type() const noexcept {
        return counter;
    }
};

} /* namespace fugax::test::helpers */

#endif /* FUGAX_TEST_HELPERS_HPP */
