#ifndef UTILS_RESOURCE_GUARD_HPP
#define UTILS_RESOURCE_GUARD_HPP

#include <optional>

namespace utils {

template<class T_resource>
class resource_guard {
    std::optional<T_resource> resource;

public:
    resource_guard() : resource { std::nullopt } {  }
    resource_guard(T_resource &&resource) : 
        resource { std::move(resource) }
        {  }
    resource_guard(resource_guard<T_resource> &&other) : 
        resource { std::move(other.resource) }
        { other.resource.reset(); }
    resource_guard(const resource_guard<T_resource> &) = delete;
    ~resource_guard() { if(resource) resource->release(); }

    resource_guard& operator=(T_resource &&other) {
        if(resource) resource->release();
        resource.emplace(std::move(other));
        return *this;
    }

    void reset() noexcept {  
        if(resource) {
            resource->release();
            resource.reset();
        }
    }

    resource_guard &operator=(resource_guard<T_resource> &&) = default;
    resource_guard &operator=(const resource_guard<T_resource> &) = delete;
    operator bool() const { return resource.has_value(); }
};

} /* namespace utils */

#endif /* UTILS_RESOURCE_GUARD_HPP */