/**
 * @file utils/include/utils/test-helpers.hpp
 * @brief Test helpers and utilities
 * @author André Medeiros
 * @date 27/06/23
 * @copyright 2023 (C) André Medeiros
**/

#ifndef UTILS_TEST_HELPERS_HPP
#define UTILS_TEST_HELPERS_HPP

#include <exception>
#include <optional>
#include <variant>

namespace utils::test_helpers {

template<class T>
class safe_result {
    using stored_type = std::variant<T, std::exception_ptr>;
    stored_type value;

public:
    using value_type = T;

    constexpr explicit safe_result(T &&value = {  })
        noexcept(std::is_nothrow_constructible_v<stored_type, T &&>) :
        value { std::move(value) }
    {  }

    constexpr explicit safe_result(const T &value = {  })
        noexcept(std::is_nothrow_constructible_v<stored_type , const T &>) :
        value { value }
    {  }

    constexpr explicit safe_result(std::exception_ptr &&error)
        noexcept(std::is_nothrow_constructible_v<stored_type, std::exception_ptr &&>) :
        value { std::move(error) }
    {  }

    constexpr explicit safe_result(const std::exception_ptr &&error)
        noexcept(std::is_nothrow_constructible_v<stored_type , const std::exception_ptr &>) :
        value { error }
    {  }

    template<class ...T_args>
    constexpr safe_result(std::in_place_t, T_args &&...args)
        noexcept(std::is_nothrow_constructible_v<T, T_args...>) :
        value { std::in_place_type_t<T>(), { std::forward<T_args>(args)... } }
    {  }

    constexpr bool has_value() const noexcept {
        return std::holds_alternative<T>(value);
    }

    constexpr bool has_error() const noexcept {
        return std::holds_alternative<std::exception_ptr>(value);
    }

    constexpr T &get_value() {
        return std::get<T>(value);
    }

    constexpr const T &get_value() const {
        return std::get<T>(value);
    }

    template<class T_error>
    T_error &get_error() const try {
        std::rethrow_exception(std::get<std::exception_ptr>(value));
    }
    catch(T_error &t) { return t; }

    template<class T_value>
    constexpr bool holds_value() const noexcept {
        return std::is_same_v<T, T_value> && has_value();
    }

    template<class T_error>
    bool holds_error() const noexcept try {
        if(has_value()) return false;

        std::rethrow_exception(std::get<std::exception_ptr>(value));
    }
    catch(T_error &) { return true; }
    catch(...) { return false; }
};

template<>
class safe_result<void> {
    std::optional<std::exception_ptr> error;

public:
    using value_type = void;

    constexpr safe_result() noexcept = default;

    constexpr explicit safe_result(std::exception_ptr &&error)
        noexcept(std::is_nothrow_constructible_v<
            std::optional<std::exception_ptr>,
            std::exception_ptr &&
        >) :
        error { std::move(error) }
    {  }

    constexpr explicit safe_result(const std::exception_ptr &error)
        noexcept(std::is_nothrow_constructible_v<
            std::optional<std::exception_ptr>,
            const std::exception_ptr &
        >) :
        error { error }
    {  }

    constexpr bool has_value() const noexcept {
        return !has_error();
    }

    constexpr bool has_error() const noexcept {
        return error.has_value();
    }

    template<class T_error>
    T_error &get_error() const try {
        std::rethrow_exception(error.value());
    }
    catch(T_error &t) { return t; }

    template<class T_error>
    bool holds_error() const noexcept try {
        if(has_value()) return false;

        std::rethrow_exception(error.value());
    }
    catch(T_error &) { return true; }
    catch(...) { return false; }
};

template<class T_task>
safe_result<std::invoke_result_t<T_task>> attempt(T_task &&task) noexcept try {
    if constexpr(std::is_void_v<std::invoke_result_t<T_task>>) {
        task();
        return {  };
    } else {
        return safe_result<std::invoke_result_t<T_task>> { task() };
    }
}
catch(...) { return safe_result<std::invoke_result_t<T_task>> { std::current_exception() }; }

inline safe_result<void> rescue(const std::exception_ptr &error)
    noexcept(std::is_nothrow_constructible_v<safe_result<void>, const std::exception_ptr &>)
{
    return safe_result<void> { error };
}

} /* namespace utils::test_helpers */

#endif /* UTILS_TEST_HELPERS_HPP */
