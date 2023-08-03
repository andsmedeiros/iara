/**
 * @file juro/promise.hpp
 * @brief Contains the definition of promise objects and imports other juro
 * namespaces
 * @author André Medeiros
*/

#ifndef JURO_PROMISE_HPP
#define JURO_PROMISE_HPP

#include<functional>
#include <memory>
#include <stdexcept>
#include <variant>
#include "juro/helpers.hpp"
#include "juro/factories.hpp"
#include "juro/compose/all.hpp"

namespace juro {

using namespace juro::helpers;
using namespace juro::factories;
using namespace juro::compose;

class promise_interface {
private:
    /**
     * @brief Holds the current state of the promise; Once settled, it cannot be
     * changed.
     */
    promise_state state = promise_state::PENDING;

    /**
     * @brief Type-erased callback to be executed once the promise is settled.
     */
    std::function<void()> on_settle;

protected:
    promise_interface() noexcept = default;
    promise_interface(promise_state state) noexcept;
    promise_interface(const promise_interface &) = delete;
    promise_interface(promise_interface &&) noexcept = default;

    promise_interface &operator=(const promise_interface &) = delete;
    promise_interface &operator=(promise_interface &&) noexcept = default;
    virtual ~promise_interface() = default;

    void set_settle_handler(std::function<void()> &&handler) noexcept;
    void resolved() noexcept;
    void rejected();

public:
    /**
     * @brief Returns the current state of the promise. A promise is pending
     * when it does not hold anything yet; it is resolved when it holds a
     * `value_type` and is rejected when it holds a `std::exception_ptr`.
     * @return The current state of the promise.
     */
    inline promise_state get_state() const noexcept { return state; }

    /**
     * @brief Returns whether the promise is pending.
     * @return Whether the promise is pending.
     */
    inline bool is_pending() const noexcept {
        return state == promise_state::PENDING;
    }

    /**
     * @brief Return whether the promise is resolved.
     * @return Whether the promise is resolved.
     */
    inline bool is_resolved() const noexcept {
        return state == promise_state::RESOLVED;
    }

    /**
     * @brief Return whether the promise is rejected.
     * @return Whether the promise is rejected.
     */
    inline bool is_rejected() const noexcept {
        return state == promise_state::REJECTED;
    }


    /**
     * @brief Return whether the promise is either resolved or rejected.
     * @return Whether the promise is either resolved or rejected.
     */
    inline bool is_settled() const noexcept {
        return state != promise_state::PENDING;
    }

#ifdef JURO_TEST
    /**
     * @brief Helper function to determine if this promise has a settle handler
     * attached.
     * @return Whether there is a settle handler attached or not.
     */
    inline bool has_handler() const noexcept {
        return static_cast<bool>(on_settle);
    }
#endif /* JURO_TEST */

};

/**
 * @brief A promise represents a value that is not available yet.
 * @tparam T The type of the promised value; defaults to `void` if unspecified.
 */
template<class T = void>
class promise : public promise_interface {
    template<class> friend class promise;

public:
    /**
     * @brief Indicates whether this is a `void` promise type or not.
     */
    static constexpr inline bool is_void = std::is_void_v<T>;

    /**
     * @brief The promised object type.
     */
    using type = T;

    /**
     * @brief Defines a type suitable to hold this promise's value, no matter
     * its type. Maps `void` to `void_type`, every other type is mapped to 
     * itself.
     */
    using value_type = storage_type<T>;

    /**
     * @brief Represents the possible values a promise can hold: a pending
     * promise holds an `empty_type`, a resolved promise holds a `value_type` 
     * and a rejected promise holds an `std::exception_ptr`.
     */
    using settle_type = 
        std::variant<empty_type, value_type, std::exception_ptr>;

private:
    /**
     * @brief Holds the settled value or `empty_type` if the promise is pending.
     */
    settle_type value;

public:
    /**
     * @brief Constructs a pending promise.
     * @warning This should not be called directly; use `juro::make_pending()` 
     * or `juro::make_promise()` instead.
     */
    promise() = default;

    /**
     * @brief Constructs a resolved promise.
     * @warning This should not be called directly; use `juro::make_resolved()`
     * instead.
     * @tparam T_value The type of the value this promise is being resolved with.
     * @param value The value the promise is being resolved with.
     */
    template<class T_value>
    promise(resolved_promise_tag, T_value &&value) :
        promise_interface { promise_state::RESOLVED },
        value { std::forward<T_value>(value) }
        {  }

    /**
     * @brief Constructs a rejected promise.
     * @warning This should not be called directly; use `juro::make_rejected()`
     * instead.
     * @tparam T_value The type of the value this promise is being rejected with.
     * @param  value The value the promise is being rejected with. If it is 
     * *not* a `std::exception_ptr`, then it is wrapped into one.
     */
    template<class T_value>
    promise(rejected_promise_tag, T_value &&value) : 
        promise_interface { promise_state::REJECTED },
        value { rejection_value(std::forward<T_value>(value)) }
        {  }

    promise(promise &&) = delete;
    promise(const promise &) = delete;
    ~promise() noexcept = default;

    promise &operator=(promise &&) = delete;
    promise &operator=(const promise &) = delete;


    /**
     * @brief Returns the resolved value stored in the promise. If the promise
     * is not resolved, will propagate a `std::bad_variant_access` exception.
     * @return The resolved value.
     */
    value_type &get_value() {
        return std::get<value_type>(value);
    }

    /**
     * @brief Returns the rejected value stored in the promise. If the promise
     * is not rejected, will propagate a `std::bad_variant_access` exception.
     * @return The rejected value.
     */
    std::exception_ptr &get_error() {
        return std::get<std::exception_ptr>(value);
    }

    /**
     * @brief Resolves the promise with a given value. Fires the settle handler 
     * if there is one already attached.
     * @tparam T_value The value type with which to settle the promise. Must be
     * convertible to `T`.
     * @param resolved_value The value with which to settle the promise.
     */
    template<class T_value = value_type>
    void resolve(T_value &&resolved_value = {}) {
        static_assert(
            std::is_convertible_v<T_value, value_type>, 
            "Resolved value is not convertible to promise type"
        );

        if(is_settled()) {
            throw promise_error { "Attempted to resolve an already settled promise" };
        }

        value = std::forward<T_value>(resolved_value);
        resolved();
    }

    /**
     * @brief Rejects the promise with a given value. Fires the settle handler 
     * if there is one attached, otherwise throws a `juro::promise_exception`.
     * @tparam T_value The value type with which to settle the promise.
     * @param rejected_value The value with which to settle the promise. If it 
     * is not an `std::exception_ptr`, it will be stored into one.
     */
    template<class T_value = promise_error>
    void reject(T_value &&rejected_value = promise_error { "Promise was rejected" }) {
        if(is_settled()) {
            throw promise_error { "Attempted to reject an already settled promise" };
        }

        value = rejection_value(std::forward<T_value>(rejected_value));
        rejected();
    }

    /**
     * @brief Attaches a settle handler to the promise, overwriting any
     * previously attached one.
     * @tparam T_on_resolve The type of the resolve handler; should receive the 
     * promised type as parameter, preferably as a reference.
     * @tparam T_on_reject The type of the reject handler; should receive an
     * `std::exception_ptr` as parameter, preferably as a reference.
     * @param on_resolve The functor to be invoked when the promise is resolved.
     * @param on_reject The functor to be invoked when the promise is rejected.
     * @return A new promise of a type that depends on the types returned by the
     * functors provided.
     * @see `juro::helpers::chained_promise_type`
     */
    template<class T_on_resolve, class T_on_reject>
    auto then(T_on_resolve &&on_resolve, T_on_reject &&on_reject) {
        assert_resolve_invocable<T_on_resolve>();
        assert_reject_invocable<T_on_reject>();
        
        using next_value_type = 
            chained_promise_type<T, T_on_resolve, T_on_reject>;

        return make_promise<next_value_type>([&] (auto &next_promise) {
            set_settle_handler([
               this,
               next_promise,
               on_resolve = std::forward<T_on_resolve>(on_resolve),
               on_reject = std::forward<T_on_reject>(on_reject)
           ] {
                try {
                    if(is_resolved()) {
                        handle_resolve(on_resolve, next_promise);
                    } else if(is_rejected()) {
                        handle_reject(on_reject, next_promise);
                    }
                } catch(...) {
                    next_promise->reject(std::current_exception());
                }
            });
        });
    }

    /**
     * @brief Attaches a resolve handler to the promise, overwriting any 
     * previously attached one. In case of rejection, the error will be 
     * propagated down the promise chain.
     * @tparam T_on_resolve The type of the resolve handler; should receive the
     * promise type as parameter, preferably by reference.
     * @param on_resolve The functor to be invoked when the promise is resolved.
     * @return A new promise of a type that depends on the type returned by the
     * functor provided.
     * @see `juro::helpers::chained_promise_type`
     */
    template<class T_on_resolve>
    inline auto then(T_on_resolve &&on_resolve) {
        return then(
            std::forward<T_on_resolve>(on_resolve),
            [] (auto &error) -> resolve_result_t<T, T_on_resolve> { std::rethrow_exception(error); }
        );
    }

    /**
     * @brief Attaches a reject handler to the promise, overwriting any
     * previously attached one. If resolved, the value will be passed down
     * along the promise chain.
     * @tparam T_on_reject The type of the reject handler; should receive a
     * `std::exception_ptr` as parameter, preferably by reference.
     * @param on_reject The functor to be invoked when the promise is rejected.
     * @return A new promise of a type that depends on the type returned by the
     * functor provided.
     * @see `juro::helpers::chained_promise_type`
     */
    template<class T_on_reject>
    inline auto rescue(T_on_reject &&on_reject) {
        if constexpr(is_void) {
            return then([] () noexcept {}, std::forward<T_on_reject>(on_reject));
        } else {
            return then(
                [] (auto &value) noexcept { return value; },
                std::forward<T_on_reject>(on_reject)
            );
        }
    }

    /**
     * @brief Attaches a settle handler to the promise, overwriting any
     * previously attached one. The handler will be invoked upon settling
     * whether the promise is resolved or rejected.
     * @tparam T_on_settle The type of the settle handler; should receive as
     * parameter a `juro::helpers::finally_argument_t`, preferably by reference.
     * @return A new promise of a type that depends on the type returned by the
     * functor provided.
     * @see `juro::helpers::chained_promise_type`
     * @see `juro::helpers::finally_argument_t`
     */
    template<class T_on_settle>
    inline auto finally(T_on_settle &&on_settle) {
        assert_settle_invocable<T_on_settle>();

        if constexpr(is_void) {
            return then(
                [=] { return on_settle(std::nullopt); }, 
                std::forward<T_on_settle>(on_settle)
            );
        } else {
            return then(on_settle, on_settle);
        }
    }

private:
    /**
     * @brief Asserts that a particular callable type is suitable to be invoked
     * when the promise is resolved.
     * @tparam T_on_resolve The callable type.
     */
    template<class T_on_resolve>
    static inline void assert_resolve_invocable() noexcept {
        static_assert(
            (is_void && std::is_invocable_v<T_on_resolve>) ||
            std::is_invocable_v<T_on_resolve, value_type &>,
            "Resolve handler has an incompatible signature."
        );
    }

    /**
     * @brief Asserts that a particular callable type is suitable to be invoked
     * when the promise is rejected.
     * @tparam T_on_resolve The callable type.
     */
    template<class T_on_reject>
    static inline void assert_reject_invocable() noexcept {
        static_assert(
            std::is_invocable_v<T_on_reject, std::exception_ptr &>,
            "Reject handler has an incompatible signature."
        );
    }

    /**
     * @brief Asserts that a particular callable type is suitable to be invoked
     * when the promise is settled.
     * @tparam T_on_resolve The callable type.
     */
    template<class T_on_settle>
    static inline void assert_settle_invocable() noexcept {
        static_assert(
            std::is_invocable_v<T_on_settle, finally_argument_t<T> &>,
            "Settle handler has an incompatible signature."
        );
    }

    /**
     * @brief Handles promise resolution, calling the resolve handler and 
     * resolving the chained promise.
     * @tparam T_on_resolve The type of the resolve handler
     * @tparam T_next_promise The type of the chained promise
     * @param on_resolve The resolve handler to be invoked
     * @param next_promise The chained promise
     */
    template<class T_on_resolve, class T_next_promise>
    void handle_resolve(T_on_resolve &on_resolve, T_next_promise &next_promise) {
        if constexpr(is_void) {
            if constexpr(resolves_void_v<T, T_on_resolve>) {
                on_resolve();
                next_promise->resolve();
            }
            if constexpr(resolves_value_v<T, T_on_resolve>) {
                next_promise->resolve(on_resolve());
            }
            if constexpr(resolves_promise_v<T, T_on_resolve>) {
                on_resolve()->pipe(next_promise);
            }
        } else {
            if constexpr(resolves_void_v<T, T_on_resolve>) {
                on_resolve(std::get<T>(value));
                next_promise->resolve();
            }
            if constexpr(resolves_value_v<T, T_on_resolve>) {
                next_promise->resolve(on_resolve(std::get<T>(value)));
            }
            if constexpr(resolves_promise_v<T, T_on_resolve>) {
                on_resolve(std::get<T>(value))->pipe(next_promise);
            }

        }
    }

    /**
     * @brief Handler promise rejection, calling the reject handler and
     * resolving the chained promise.
     * @tparam T_on_reject The type of the reject handler
     * @tparam T_next_promise The type of the chained promise
     * @param on_reject The reject handler to be invoked
     * @param next_promise The chained promise
     */
    template<class T_on_reject, class T_next_promise>
    void handle_reject(T_on_reject &&on_reject, T_next_promise &next_promise) {
        if constexpr(rejects_void_v<T_on_reject>) {
            on_reject(std::get<std::exception_ptr>(value));
            next_promise->resolve();
        }
        if constexpr(rejects_value_v<T_on_reject>) {
            auto &rejected_value = std::get<std::exception_ptr>(value);
            next_promise->resolve(on_reject(rejected_value));
        }
        if constexpr(rejects_promise_v<T_on_reject>) {
            auto &rejected_value = std::get<std::exception_ptr>(value);
            on_reject(rejected_value)->pipe(next_promise);

        }
    }

    /**
     * @brief Prepares a value for rejection: wraps the supplied value into a 
     * `std::exception_ptr` unless it already is one, in which case it is simply
     * returned.
     * @tparam T_value The type of value to be prepared
     * @param value The value to be prepared for rejection
     * @return A `std::exception_ptr` suitable for promise rejection.
     */
    template<class T_value>
    inline auto rejection_value(T_value &&value) {
        using bare_type = std::remove_cv_t<std::remove_reference_t<T_value>>;
        if constexpr(std::is_same_v<bare_type, std::exception_ptr>) {
            return std::forward<T_value>(value);
        } else {
            return std::make_exception_ptr(std::forward<T_value>(value));
        }
    }
    
    /**
     * @brief Pipes a promise into another: when the current promise is settled,
     * the next will be too with the same state and value.
     * @tparam T_next_promise The target promise type
     * @param next_promise the The target promise
     */
    template<class T_next_promise>
    inline void pipe(T_next_promise &&next_promise) {
        if constexpr(is_void) {
            then(
                [=] { next_promise->resolve(); },
                [=] (auto &error) { next_promise->reject(std::move(error)); }
            );
        } else {
            then(
                [=] (auto &value) { next_promise->resolve(std::move(value)); },
                [=] (auto &error) { next_promise->reject(std::move(error)); }
            );
        }
    }
    
#ifdef JURO_TEST
public:
    /**
     * @brief Helper function to determine if this promise holds a determinate 
     * value type.
     * @tparam T_value The type being evaluated.
     * @return Whether the promise's value container holds a value of type 
     * `T_value` or not.
     */
    template<class T_value>
    inline bool holds_value() const noexcept { 
        return std::holds_alternative<T_value>(value); 
    }

    /**
     * @brief Helper function to determine if this promise holds no meaningful 
     * value, i.e., an `empty_type`.
     * @return Whether this promise's value container holds a value of type
     * `empty_type` or not.
     */
    inline bool is_empty() const noexcept {
        return std::holds_alternative<empty_type>(value);
    }

#endif /* JURO_TEST */
};

} /* namespace juro */

#endif /* JURO_PROMISE_HPP */