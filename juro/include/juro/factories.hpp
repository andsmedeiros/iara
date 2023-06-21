/**
 * @file juro/factories.hpp
 * @brief Contains factory functions for creating promises in all possible 
 * states.
 * @author André Medeiros
*/

#ifndef JURO_FACTORIES_HPP
#define JURO_FACTORIES_HPP

#include "juro/helpers.hpp"

namespace juro::factories {

using namespace juro::helpers;

/**
 * @brief Creates a new promise and supplies it to the provided launcher 
 * functor. This promise can than be dispatched elsewhere.
 * @tparam T The type of the promise being created
 * @tparam T_launcher The type of the launched functor
 * @param launcher The launched functor
 * @return The newly created promise
 */
template<class T = void, class T_launcher>
auto make_promise(T_launcher &&launcher) {
    static_assert(
        std::is_invocable_v<T_launcher, const promise_ptr<T> &>,
        "Launcher function has an incompatible signature."
    );
    const auto p = std::make_shared<promise<T>>();
    launcher(p);
    return p;
}

/**
 * @brief Creates a new pending promise.
 * @tparam T The type of the promis ebeing created
 * @return The newly created promise
 */
template<class T = void>
auto make_pending() {
    return std::make_shared<promise<T>>();
}

/**
 * @brief Creates a new non-void resolved promise.
 * @tparam T The type of the promise being created. Unless explicitly supplied,
 * will be inferred from the `value` parameter.
 * @param value The value with which to resolve the promise
 * @return The newly created promise
 */
template<class T>
auto make_resolved(T &&value) {
    return std::make_shared<promise<bare_t<T>>>(
        resolved_promise_tag {  }, 
        std::forward<T>(value)
    );
}

/**
 * @brief Creates a new void resolved promise.
 * @return The newly created promise
 */
inline auto make_resolved() { 
    return std::make_shared<promise<void>>(
        resolved_promise_tag {  }, 
        void_type {  }
    );
}

/**
 * @brief Creates a new rejected promise.
 * @note This is the only supported way of creating a rejected promise without
 * an attached settle handler. Creating a pending promise and immediately 
 * rejecting it would throw a `promise_error` with message 
 * "Unhandled promise rejection".
 * @tparam T The type of the promise being created. If unsupplied, defaults to 
 * `void`
 * @tparam T_value The type of the value with which to reject the promise
 * @param value The value with which to reject the promise
 * @return The newly create promise
 */
template<class T = void, class T_value = promise_error>
auto make_rejected(T_value &&value = T_value { "Promise was rejected" }) {
    return std::make_shared<promise<bare_t<T>>>(
        rejected_promise_tag {  }, 
        std::forward<T_value>(value)
    );
}

} /* namespace juro::factories */

#endif /* JURO_FACTORIES_HPP */