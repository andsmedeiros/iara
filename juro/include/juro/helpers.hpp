/**
 * @file juro/helpers.hpp
 * @brief Contains type helpers used throughout the library
 * @author André Medeiros
*/

#ifndef JURO_HELPERS_HPP
#define JURO_HELPERS_HPP

#include <memory>
#include <optional>
#include <type_traits>
#include <stdexcept>
#include <variant>

namespace juro {
template<class> class promise;
} /* namespace juro */

namespace juro::helpers {

/**
 * @brief A shared pointer to a `promise<T>`
 * @tparam T The type of the promised value
 */
template<class T>
using promise_ptr = std::shared_ptr<promise<T>>;

/**
 * @brief The exception error thrown by invalid promise operations
 */
struct promise_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/**
 * @brief The possible states of a promise at one given time
 */
enum class promise_state { PENDING, RESOLVED, REJECTED };

/**
 * @brief Tag type to disambiguate the settled promise constructors call. This
 * represents the resolved state.
 */
struct resolved_promise_tag {  };

/**
 * @brief Tag type to disambiguate the settled promise constructors call. This
 * represents the rejected state.
 */
struct rejected_promise_tag {  };

/**
 * @brief Tag struct to represent an absent value. This is used by pending 
 * promises as a value placeholder.
 */
struct empty_type {  };

/**
 * @brief Tag struct to represent a void value. This is used by void promises
 * as a value placeholder.
 */
struct void_type {  
    friend inline bool operator==(const void_type &, const void_type &) noexcept {
        return true; 
    }
};

/**
 * @brief Defines a concrete type that can hold the value of a promise.
 * @details As `void` is an incomplete type, if `promise<void>` had a `void` 
 * member, the program would be ill-formed. This alias maps `void` to 
 * `void_type` and every other type to itself, so both void and non-void 
 * promises can have a value member.
 * @tparam T Type type of the promise
 */
template<class T>
using storage_type = std::conditional_t<std::is_void_v<T>, void_type, T>;

/**
 * @brief Defines a container suitable to hold two different types. 
 * @details This clause activates when both types are diferent. The internal 
 * `type` is a `std::variant<T1, T2>`.
 * @tparam T1 
 * @tparam T2
 */
template<class T1, class T2, class = void>
struct common_container {
    using type = std::variant<T1, T2>;
};

/**
 * @brief Defines a container suitable to hold two different types. 
 * @details This clause activates when the first type is `void`. The internal 
 * `type` is a `std::optional<T>`.
 * @tparam T The non-void type
 */
template<class T>
struct common_container<void, T> {  
    using type = std::optional<T>;
};

/**
 * @brief Defines a container suitable to hold two different types. 
 * @details This clause activates when the second type is `void`. The internal 
 * `type` is a `std::optional<T>`.
 * @tparam T The non-void type
 */
template<class T>
struct common_container<T, void> {  
    using type = std::optional<T>;
};

/**
 * @brief Defines a container suitable to hold two different types. 
 * @details This clause activates both types are `void`. The internal `type` 
 * is a `void`.
 * @tparam T The non-void type
 */
template<>
struct common_container<void, void> {  
    using type = void;
};

/**
 * @brief Defines a container suitable to hold two different types. 
 * @details This clause activates when both types are the same. The internal 
 * `type` is a `T`.
 * @tparam T The type supplied to both parameters
 */
template<class T>
struct common_container<T, T> {
    using type = T;
};

/**
 * @brief Helper type that aliases `common_container<T1, T2>::type`.
 * @tparam T1 
 * @tparam T2 
 */
template<class T1, class T2>
using common_container_t = typename common_container<T1, T2>::type;

/**
 * @brief Yields a non-promise type suitable for chained promise resolution.
 * This clause activates when the provided type is already not a promise.
 * @tparam T The type to attempt unwrapping
 */
template<class T>
struct unwrap_if_promise {
    using type = T;
};

/**
 * @brief Yields a non-promise type suitable for chained promise resolution.
 * This clause activates when the provided type is a promise and aliases the
 * promises's type.
 * @tparam T The type to attempt unwrapping
 */
template<class T>
struct unwrap_if_promise<promise_ptr<T>> {
    using type = typename unwrap_if_promise<T>::type;
};

/**
 * @brief Helper alias to `unwrap_if_promise<T>::type`
 * @tparam T The type to attempt unwrapping
 */
template<class T>
using unwrap_if_promise_t = typename unwrap_if_promise<T>::type;

/**
 * @brief Type trait to detect if a type is a promise. This clause activates
 * when supplied with a non-promise type.
 * @tparam T The type to inspect
 */
template<class T>
struct is_promise : public std::false_type { };

/**
 * @brief Type trait to detect if a type is a promise. This clause activates
 * when supplied with a promise type.
 * @tparam T The type to inspect
 */
template<class T>
struct is_promise<promise_ptr<T>> : public std::true_type {  };

/**
 * @brief Helper constexpr bool to detect if a given type is a promise
 * @tparam T The type to inspect
 */
template<class T>
static constexpr inline bool is_promise_v = is_promise<T>::value;

/**
 * @brief Type trait to determine what type does a resolve handler returns. This
 * clause activates for non-void promises.
 * @tparam T The promise type
 * @tparam T_on_resolve The resolve handler type
 */
template<class T, class T_on_resolve>
struct resolve_result {
    using type = std::invoke_result_t<T_on_resolve, T &>;
};

/**
 * @brief Type trait to determine what type does a resolve handler returns. This
 * clause activates for void promises.
 * @tparam T_on_resolve The resolve handler type
 */
template<class T_on_resolve>
struct resolve_result<void, T_on_resolve> {
    using type = std::invoke_result_t<T_on_resolve>;
};

/**
 * @brief Helper alias to `resolve_result<T, T_on_resolve>::type`
 * @tparam T The promise type
 * @tparam T_on_resolve The resolve handler type
 */
template<class T, class T_on_resolve>
using resolve_result_t = typename resolve_result<T, T_on_resolve>::type;

/**
 * @brief Helper alias to 
 * `std::invoke_result_t<T_on_reject, std::exception_ptr &>`. Yields the type a 
 * reject handler returns.
 * @tparam T_on_reject The reject handler type
 */
template<class T_on_reject>
using reject_result_t = std::invoke_result_t<T_on_reject, std::exception_ptr &>;

/**
 * @brief Determines the type of the promise that should be returned by a call 
 * to `.then()`, `rescue()` or `finally()`.
 * @details When calling `.then()`, `.rescue()` or `.finally()`, a new promise
 * is returned. The type of this promise is determined based on what each 
 * attached handler returns, as follows:
 * 
 * - The resolve and reject handlers' returning types are extracted as T1 and 
 * T2.
 * - T1 and T2 are unwrapped into U1 and U2: if any of them is a promise, it is 
 * mapped to the promise's type, otherwise, it is mapped to itself.
 * - If U1 and U2 are the same (U1 == U2 == U), the next promise's type is U. 
 * - Otherwise, if either U1 or U2 is `void`, the next promise's type is a 
 * `std::optional<UX>`, where X is the non-void type. 
 * - If instead U1 and U2 are both non-void and different, the next promise's 
 * type is a `std::variant<U1, U2>`.
 * @tparam T The promise type
 * @tparam T_on_resolve The supplied resolve handler type
 * @tparam T_on_reject The supplied reject handler type
 */
template<class T, class T_on_resolve, class T_on_reject>
using chained_promise_type = common_container_t<
    unwrap_if_promise_t<resolve_result_t<T, T_on_resolve>>,
    unwrap_if_promise_t<reject_result_t<T_on_reject>>
>;

/**
 * @brief Determines the type of the parameter that a `.finally()` handler takes.
 * @details When calling `.finally()`, a single handler is called whether the
 * promise is resolved or rejected. Its parameter's type is determined as 
 * follows:
 * 
 * - If the promise type is void, the parameter type is a 
 * `std::optional<std::exception_ptr>`.
 * - If the promise type T is non-void, the parameter type is a
 * `std::variant<T, std::exception_ptr>`.
 * @tparam T The promise type
 */
template<class T>
using finally_argument_t = common_container_t<T, std::exception_ptr>;


/**
 * @brief Helper constexpr bool to detect if a resolve handler returns void.
 * @tparam T The promise type
 * @tparam T_on_resolve The type of the resolve handler
 */
template<class T, class T_on_resolve>
static constexpr inline bool resolves_void_v = 
    std::is_void_v<resolve_result_t<T, T_on_resolve>>;

/**
 * @brief Helper constexpr bool to detect if a resolve handler returns a 
 * promise.
 * @tparam T The promise type
 * @tparam T_on_resolve The type of the resolve handler
 */
template<class T, class T_on_resolve>
static constexpr inline bool resolves_promise_v = 
    !std::is_void_v<resolve_result_t<T, T_on_resolve>> &&
    is_promise_v<resolve_result_t<T, T_on_resolve>>;

/**
 * @brief Helper constexpr bool to detect if a resolve handler returns a 
 * non-promise value.
 * @tparam T The promise type
 * @tparam T_on_resolve The type of the resolve handler
 */
template<class T, class T_on_resolve>
static constexpr inline bool resolves_value_v = 
    !std::is_void_v<resolve_result_t<T, T_on_resolve>> &&
    !is_promise_v<resolve_result_t<T, T_on_resolve>>;

/**
 * @brief Helper constexpr bool to detect if a reject handler returns void.
 * @tparam T_on_reject The type of the reject handler
 */
template<class T_on_reject>
static constexpr inline bool rejects_void_v = 
    std::is_void_v<reject_result_t<T_on_reject>>;

/**
 * @brief Helper constexpr bool to detect if a reject handler returns a promise.
 * @tparam T_on_reject The type of the reject handler
 */
template<class T_on_reject>
static constexpr inline bool rejects_promise_v = 
    !std::is_void_v<reject_result_t<T_on_reject>> &&
    is_promise_v<reject_result_t<T_on_reject>>;

/**
 * @brief Helper constexpr bool to detect if a reject handler returns a 
 * non-promise value.
 * @tparam T_on_reject The type of the reject handler
 */
template<class T_on_reject>
static constexpr inline bool rejects_value_v = 
    !std::is_void_v<reject_result_t<T_on_reject>> &&
    !is_promise_v<reject_result_t<T_on_reject>>;
    
/**
 * @brief Yields the unwrapped type returned by a resolve handler
 * @tparam T The promise type
 * @tparam T_on_resolve The type of the resolve handler
 */
template<class T, class T_on_resolve>
using resolved_promise_value_t = 
    unwrap_if_promise_t<resolve_result_t<T, T_on_resolve>>;
    
/**
 * @brief Yields the unwrapped type returned by a reject handler
 * @tparam T_on_resolve The type of the reject handler
 */
template<class T_on_resolve>
using rejected_promise_value_t = 
    unwrap_if_promise_t<reject_result_t<T_on_resolve>>;

/**
 * @brief Store unique types into a tuple. Default clause for already unique 
 * tuples.
 * @tparam T_tuple The tuple type
 * @tparam ... The types to filter
 */
template<class T_tuple, class ...>
struct unique { 
    using type = T_tuple; 
};

/**
 * @brief Stores unique types into a tuple. This clause is activated when at
 * least one type is yet unfiltered in a recursive manner.
 * @tparam ...T_values The unique stored types
 * @tparam T The type being currently tested
 * @tparam ...T_rest The rest of the unfiltered types
 */
template<class ...T_values, class T, class ...T_rest>
struct unique<std::tuple<T_values...>, T, T_rest...> {
    using type = std::conditional_t<
        std::disjunction_v<std::is_same<T, T_values>...>,
        typename unique<std::tuple<T_values...>, T_rest...>::type,
        typename unique<std::tuple<T_values..., T>, T_rest...>::type
    >;
};

/**
 * @brief Stores unique types into a tuple. This is the trampoline 
 * implementation that delegates to the others.
 * @tparam ...T_values The values to filter
 */
template<class ...T_values>
struct unique<std::tuple<>, std::tuple<T_values...>> : 
    unique<std::tuple<>, T_values...> {  };

/**
 * @brief Helper type that aliases `unique<T_values...>::type`. This defines
 * a tuple containing only unrepeated types of `T_values...`.
 * @tparam ...T_values The types to filter.
 */
template<class ...T_values>
using unique_t = typename unique<std::tuple<>, std::tuple<T_values...>>::type;

/**
 * @brief Custom implementation of `std::remove_cvref_t`
 * @tparam T The type to be sanitised
 */
template<class T>
using bare_t = std::remove_reference_t<std::remove_cv_t<T>>;

} /* namespace juro::helpers */

#endif /* JURO_HELPERS_HPP */