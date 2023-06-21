#ifndef UTILS_STORAGE_FOR_HPP
#define UTILS_STORAGE_FOR_HPP

#include <new>

namespace utils {

template<class T_object>
class storage_for {
    union storage_space {
        T_object object;
        struct empty_storage {  } empty;
        ~storage_space() {  }
    } storage;

public:
    storage_for() : storage { .empty {  } } {  }
    storage_for(auto && ...args) : 
        storage { T_object { std::forward<decltype(args)>(args)... } }
        {  }
    ~storage_for() = default;
    storage_for(const storage_for<T_object> &) = delete;
    storage_for(storage_for<T_object> &&) = delete;
    storage_for &operator=(const storage_for<T_object> &) = delete;
    storage_for &operator=(storage_for<T_object> &&) = delete;

    T_object *construct(auto && ...args) {
        return new (&storage.object)
            T_object { std::forward<decltype(args)>(args)... };
    }

    storage_for<T_object> *destruct() noexcept { 
        storage.object.~T_object(); 
        new (&storage.empty) typename storage_space::empty_storage {  };
        return this;
    }

    T_object extract() noexcept {
        T_object extracted { std::move(storage.object) };
        destruct();
        return extracted;
    }

    T_object &operator->() noexcept { return storage.object; }

    inline static storage_for<T_object> &from(T_object *object) noexcept {
        return *reinterpret_cast<storage_for<T_object> *>(object);
    }
};

} /* namespace utils */

#endif /* UTILS_STORAGE_FOR_HPP */