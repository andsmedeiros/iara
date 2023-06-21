#include "utils/format.hpp"

#ifndef _GNU_SOURCE
#error "Macro _GNU_SOURCE must be defined for formatting support"
#endif

namespace utils {

[[gnu::format(printf, 1, 2)]]
std::string format(const char *fmt, ...) {
    std::va_list scan_args;
    va_start(scan_args, fmt);
    
    char *formatted;
    std::size_t size = vasprintf(&formatted, fmt, scan_args);
    std::string result { formatted, size };
    free(formatted);
    return result;
}

} /* namespace utils */