#ifndef UTILS_LOGGER_HPP
#define UTILS_LOGGER_HPP

#include <cstdio>
#include <cstdarg>
#include "types.hpp"

#ifdef NEBUG
#define LOGGER_MOCK 1
#else /* NDEBUG */
#define LOGGER_MOCK 0
#endif /* NDEBUG */

namespace utils {

using namespace types;

class logger_scope {
public:
    enum class entry_level {
        DEBUG = 0,
        INFO,
        WARN,
        ERROR,
        FATAL
    };

private:
    const char * const module;
    volatile u32 &timer;
    entry_level current_level;
    
    constexpr static const char * entry_tags[] = {
        "DEBUG",
        "INFO ",
        "WARN ",
        "ERROR",
        "FATAL"
    };
    
        
    void log(entry_level level, const char *format, va_list &args) {
        if(LOGGER_MOCK || level < current_level) return;
        
        char buffer[1024];
        int pos = sprintf(buffer, "[%08u][%s][%-12s] ", timer, entry_tags[static_cast<std::size_t>(level)], module);
        if(pos > 0) {
            vsprintf(&buffer[pos], format, args);
            puts(buffer);
        }
    }
    
public:
    
    inline logger_scope(const char *module, volatile u32 &timer, entry_level level = entry_level::INFO) : 
        module(module), timer(timer), current_level(level) {  }

    inline void set_level(entry_level level) { current_level = level; }
    
    void debug(const char *format, ...) {
        va_list args;
        va_start(args, format);
        log(entry_level::DEBUG, format, args);
    }
    
    void info(const char *format, ...) {
        va_list args;
        va_start(args, format);
        log(entry_level::INFO, format, args);
    }
    
    void warn(const char *format, ...) {
        va_list args;
        va_start(args, format);
        log(entry_level::WARN, format, args);
    }
    
    void error(const char *format, ...) {
        va_list args;
        va_start(args, format);
        log(entry_level::ERROR, format, args);
    }
    
    void fatal(const char *format, ...) {
        va_list args;
        va_start(args, format);
        log(entry_level::FATAL, format, args);
    }
    
};

}; /* namespace utils */

#endif /* UTILS_LOGGER_HPP */