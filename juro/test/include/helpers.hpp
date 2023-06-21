#ifndef JURO_TEST_HELPERS_HPP
#define JURO_TEST_HELPERS_HPP

#include <optional>
#include <stdexcept>
#include <variant>

namespace juro::test::helpers {

template<class T>
class safe_result {
    std::variant<storage_type<T>, std::exception_ptr> value;

public:
    using value_type = T;

    template<class T_value>
    safe_result(T_value &&value) : 
        value { std::forward<T_value>(value) }
        {  }

    inline bool has_value() const noexcept { 
        return std::holds_alternative<storage_type<T>>(value); 
    }

    inline bool has_error() const noexcept {
        return std::holds_alternative<std::exception_ptr>(value);
    }

    const storage_type<T> &get_value() const { 
        return std::get<storage_type<T>>(value); 
    }

    template<class T_error>
    T_error &get_error() const try { 
        std::rethrow_exception(std::get<std::exception_ptr>(value));
    }
    catch(T_error &t) { return t; }

    template<class T_error>
    bool holds_error() const noexcept try {
        if(has_value()) return false;

        std::rethrow_exception(std::get<std::exception_ptr>(value));
    }
    catch(T_error &) { return true; }
    catch(...) { return false; }
};

template<class T_task>
safe_result<std::invoke_result_t<T_task>> attempt(T_task &&task) noexcept try {
    if constexpr(std::is_void_v<std::invoke_result_t<T_task>>) {
        task();
        return void_type {  };
    } else {
        return task();
    }
} 
catch(...) { return std::current_exception(); }

inline safe_result<void> rescue(std::exception_ptr &error) {
    return error;
}

} /* namespace juro::test::helpers */

#endif /* JURO_TEST_HELPERS_HPP */