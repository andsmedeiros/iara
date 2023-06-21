#ifndef UTILS_TYPE_TRAITS_HPP
#define UTILS_TYPE_TRAITS_HPP

#include <tuple>
#include <type_traits>

namespace utils::type_traits {
template<class T_object, class T_void = void, class ...T_args>
struct is_list_constructible_impl : std::false_type {  };

template<class T_object, class ...T_args>
struct is_list_constructible_impl<
    T_object, 
    std::void_t<decltype(T_object { std::declval<T_args>()... })>,
    T_args ...
> : std::true_type {  };

template<class T_object, class ...T_args>
struct is_list_constructible : std::integral_constant<
    bool, 
    is_list_constructible_impl<T_object, T_args ...>::value
> {  };
template<class T_object, class ...T_args>
inline constexpr bool is_list_constructible_v = 
    is_list_constructible<T_object, T_args...>::value;

template<class T_class, class T_tuple>
struct contains;

template<class T_class, class ...T_tuple_elements>
struct contains<T_class, std::tuple<T_tuple_elements...>> : std::integral_constant<
    bool,
    std::disjunction_v<std::is_same_v<T_class, T_tuple_elements>...>
> {  };

template<class T_class, class T_tuple>
constexpr inline bool contains_v = contains<T_class, T_tuple>::value;

template<class T_tuple, class ...>
struct unique { 
    using type = T_tuple; 
};

template<class ...T_values, class T, class ...T_rest>
struct unique<std::tuple<T_values...>, T, T_rest...> {
    using type = std::conditional_t<
        std::disjunction_v<std::is_same<T, T_values>...>,
        typename unique<std::tuple<T_values...>, T_rest...>::type,
        typename unique<std::tuple<T_values..., T>, T_rest...>::type
    >;
};

template<class ...T_values>
struct unique<std::tuple<>, std::tuple<T_values...>> : 
    unique<std::tuple<>, T_values...> {  };

template<class ...T_values>
using unique_t = typename unique<std::tuple<>, std::tuple<T_values...>>::type;

} /* namespace utils::type_traits */

#endif /* UTILS_TYPE_TRAITS_HPP */