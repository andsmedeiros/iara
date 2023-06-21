#ifndef JURO_COMPOSE_ALL_HPP
#define JURO_COMPOSE_ALL_HPP

#include <memory>
#include <optional>
#include "juro/helpers.hpp"
#include "juro/factories.hpp"

namespace juro::compose {

using namespace juro::helpers;
using namespace juro::factories;

template<class ...T_values>
using all_result = std::tuple<storage_type<T_values>...>;

template<class ...T_values>
class all_coordinator : public std::enable_shared_from_this<all_coordinator<T_values...>> {
    using result_type = all_result<T_values...>;
    using transient_type = std::tuple<std::optional<storage_type<T_values>>...>;

    transient_type working_area;
    std::size_t counter = sizeof...(T_values);
    promise_ptr<result_type> promise;

public:
    all_coordinator(const promise_ptr<result_type> &promise) :
        promise { promise }
    {  }

    template<std::size_t ...Indices>
    void attach(std::index_sequence<Indices...>, std::tuple<const promise_ptr<T_values> &...> &&promises) {
        ([&, this] (auto &promise, auto &slot) {
            using promise_type =
                typename std::remove_reference_t<decltype(promise)>::element_type;
            if constexpr(promise_type::is_void) {
                promise->then(
                    [this, &slot, guard = this->shared_from_this()] {
                        on_resolve(void_type {}, slot);
                    },
                    [&] (std::exception_ptr &error) { on_reject(error); }
                );
            } else {
                promise->then(
                    [this, &slot, guard = this->shared_from_this()] (auto &value) {
                        on_resolve(value, slot);
                    },
                    [&] (std::exception_ptr &error) { on_reject(error); }
                );
            }
        }(std::get<Indices>(promises), std::get<Indices>(working_area)), ...);
    }

    template<class T_value>
    void on_resolve(T_value &&value, std::optional<std::remove_reference_t<T_value>> &slot) {
        slot = std::forward<T_value>(value);

        if(--counter == 0 && promise->is_pending()) {
            promise->resolve(std::apply([] (auto &...values) {
                return std::make_tuple(std::move(values.value())...);
            }, working_area));
        }
    }

    void on_reject(std::exception_ptr &error) {
        if(promise->is_pending()) {
            promise->reject(error);
        }
    }
};

void void_settler(const promise_ptr<void> &promise, const promise_ptr<void> &all_promise, const std::shared_ptr<std::size_t> &counter);

template<class ...T_values>
auto all(const promise_ptr<T_values> &...promises) {
    if constexpr(std::conjunction_v<std::is_void<T_values>...>) {
        return make_promise<void>([&] (const auto &all_promise) {
            auto counter = std::make_shared<std::size_t>(sizeof...(T_values));
            for(const auto &promise : std::array<std::reference_wrapper<const promise_ptr<void>>, sizeof...(T_values)> { promises... }) {
                void_settler(promise, all_promise, counter);
            }
        });
    } else {
        return make_promise<all_result<T_values...>>([&] (const auto &all_promise) {
            auto coordinator = std::make_shared<all_coordinator<T_values...>>(all_promise);
            coordinator->attach(
                std::index_sequence_for<T_values...>(),
                std::forward_as_tuple(promises...)
            );
        });
    }
}

} /* namespace juro::compose */

#endif /* JURO_COMPOSE_ALL_HPP */