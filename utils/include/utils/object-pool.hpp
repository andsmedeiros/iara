#ifndef OBJECT_POOL_HPP
#define OBJECT_POOL_HPP


#include "object-buffer.hpp"
#include "circular-queue.hpp"
#include <vector>

template<class T, std::size_t log_factor = 3>
class object_pool {

    using object = object_buffer<T>;
    static constexpr std::size_t factor = 1<<log_factor;
    std::size_t capacity;
    std::vector<object *> blocks;
    circular_queue<object *, log_factor> queue;

public:
    object_pool() : capacity(factor) {  
        object *block = new object[factor];
        blocks.push_back(block);
        for(auto i = 0; i < factor; i++) queue.push(&block[i]);
    }

    ~object_pool() {
        for(auto block : blocks) delete[] block;
    }

    T *allocate() {
        if(queue.get_count() == 0) {
            object *block = new object[capacity];
            blocks.push_back(block);
            for(auto i = 0; i < capacity; i++) queue.push(&block[i]);
            capacity *= 2;
        }

        return reinterpret_cast<T *>(queue.pop());
    }

    void deallocate(T *obj) {
        queue.push(reinterpret_cast<object *>(obj));
    }

};

#endif /* OBJECT_POOL_HPP */