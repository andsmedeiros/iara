/**
 * @file juro/compose/race.hpp
 * @brief Contains definitions of promise races and auxiliary structures
 * @author André Medeiros
*/

#ifndef JURO_COMPOSE_RACE_HPP
#define JURO_COMPOSE_RACE_HPP

#include "juro/helpers.hpp"
#include "juro/factories.hpp"

namespace juro::compose {

using namespace juro::helpers;
using namespace juro::factories;

/**
 * @brief Base template for race result types; see concrete implementations for
 * more details
 * @tparam ... 
 */
template<class ...>
struct race_result;

template<class ...T_values>
struct race_result<std::tuple<T_values...>> {
    using type = std::variant<storage_type<T_values>...>;
};

template<class T>
struct race_result<std::tuple<T>> {
    using type = storage_type<T>;
};

template<class ...T_values>
using race_result_t = typename race_result<T_values...>::type;

template<class ...T_values>
auto race(promise_ptr<T_values> ...promises) {
    return make_promise<race_result_t<unique_t<T_values...>>>([&] (auto &race_promise) {
        ([&] (auto &promise) {
            if constexpr(std::remove_reference_t<decltype(promise)>::element_type::is_void) {
                promise->then(
                    [=] { 
                        if(race_promise->is_pending()) {
                            race_promise->resolve(void_type {  }); 
                        }
                    },
                    [=] (auto &error) {
                        if(race_promise->is_pending()) {
                            race_promise->reject(error);
                        }
                    }
                );
            } else {
                promise->then(
                    [=] (auto &value) { 
                        if(race_promise->is_pending()) {
                            race_promise->resolve(std::move(value)); 
                        }
                    },
                    [=] (auto &error) { 
                        if(race_promise->is_pending()) {
                            race_promise->reject(error); 
                        }
                    }
                );
            }
        }(promises), ...);
    });
}

} /* namespace juro::compose */

#endif /* JURO_COMPOSE_RACE_HPP */