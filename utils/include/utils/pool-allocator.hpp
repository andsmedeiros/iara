#ifndef POOL_ALLOCATOR_HPP
#define POOL_ALLOCATOR_HPP

#include "object-pool.hpp"

template<class T, std::size_t log_factor = 3>
struct pool_allocator {
    
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    static object_pool<T, log_factor> pool;

    pool_allocator() {  };
    
    template<class T_other, std::size_t log_factor_other = log_factor>
    pool_allocator(const pool_allocator<T_other, log_factor_other>& other) {  }
    
    T *allocate(std::size_t len) {
        if(len > 1) throw std::bad_alloc();
        return pool.allocate();
    }

    void deallocate(T *obj, std::size_t len) {
        pool.deallocate(obj);
    }
    
    void construct(pointer p, const_reference val) {
        new(p) T(val);
    }
    
    void destroy(pointer p) {
        p->~T();
    }
    
    size_type max_size() const noexcept { return 1; }

    template<class T_other, std::size_t log_factor_other = log_factor>
    struct rebind {
        using other = pool_allocator<T_other, log_factor_other>;
    };
};

template<class T, std::size_t log_factor>
object_pool<T, log_factor> pool_allocator<T, log_factor>::pool;


#endif /* POOL_ALLOCATOR_HPP */