#ifndef UTILS_FORMAT_HPP
#define UTILS_FORMAT_HPP

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
#include "types.hpp"

namespace utils {

[[gnu::format(printf, 1, 2)]]
std::string format(const char *fmt, ...);


template<class ...T_args>
class formatter {
    const char *fmt;

public:
    explicit constexpr formatter(const char *fmt) noexcept :
        fmt { fmt }
    {  }

    formatter(const formatter &) noexcept = default;
    formatter(formatter &&) noexcept = default;

    formatter &operator=(const formatter &) noexcept = default;
    formatter &operator=(formatter &&) noexcept = default;
    virtual ~formatter() noexcept = default;

    std::string operator()(const T_args &...args) const {
        return format(fmt, args...);
    }
};

} /* namespace utils */

#endif /* UTILS_FORMAT_HPP */