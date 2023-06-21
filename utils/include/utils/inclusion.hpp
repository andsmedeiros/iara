#ifndef UTILS_INCLUSION_HPP
#define UTILS_INCLUSION_HPP

namespace utils {

template<class ...>
struct includes {
    static constexpr bool value = false;
};

template<class T_find, class T_other, class ... T_rest>
struct includes<T_find, T_other, T_rest ...> {
    static constexpr bool value = 
        std::is_same<T_find, T_other>::value || includes<T_find, T_rest ...>::value;
};

} /* namespace utils */

#endif /* UTILS_INCLUSION_HPP */