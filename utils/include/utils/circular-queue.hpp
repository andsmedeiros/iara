#ifndef UTILS_CIRCULAR_QUEUE_HPP
#define UTILS_CIRCULAR_QUEUE_HPP

#include <memory>
#include "storage-for.hpp"

namespace utils {

template<class T_object>
class circular_queue {
    using placeholder = storage_for<T_object>;

    std::size_t head = 0, count = 0, capacity, mask;
    std::unique_ptr<placeholder[]> queue;

    inline std::size_t pos(std::size_t i, std::size_t mask) const noexcept { 
        return i & mask; 
    }
    
    inline std::size_t pos(std::size_t i) const noexcept { 
        return pos(i, mask); 
    }

    void grow() {
        std::size_t new_capacity = capacity * 2;
        auto new_queue = std::make_unique<placeholder[]>(new_capacity);

        std::size_t new_mask = new_capacity - 1;
        for(auto i = head; i != head + count; i++) {
            new_queue[pos(i, new_mask)].construct(queue[pos(i)].extract());
        }

        capacity = new_capacity;
        mask = new_mask;
        queue = std::move(new_queue);
    }

public:
    circular_queue(std::size_t factor_log2n = 3) : 
        capacity { 1UL<<factor_log2n },
        mask { capacity - 1 },
        queue { std::make_unique<placeholder[]>(capacity) }
        {  }

    ~circular_queue() {
        for(auto i = head; i < head + count; i++) {
            queue[pos(i)].destruct();
        }
    };

    circular_queue(circular_queue<T_object> &&) = default;
    circular_queue(const circular_queue<T_object> &) = delete;
    
    circular_queue<T_object> &operator=(circular_queue<T_object> &&) =
        default;
    circular_queue<T_object> &operator=(const circular_queue<T_object> &) =
        delete;
    
    inline std::size_t get_capacity() const noexcept { return capacity; }
    inline std::size_t get_count() const noexcept { return count; }
    inline bool is_empty() const noexcept { return count == 0; }

    void push(T_object object) {
        emplace(std::move(object));
    }

    void emplace(auto && ...args) {
        if(count == capacity) grow();
        queue[pos(head + count++)]
            .construct(std::forward<decltype(args)>(args)...);
    }

    T_object pop() {
        T_object object { queue[pos(head++)].extract() };
        count--;
        return object;
    }

    void swap(circular_queue &other) noexcept {
        auto temp = std::move(other);
        other = std::move(*this);
        *this = std::move(temp);
    }
};

} /* namespace utils */

#endif /* UTILS_CIRCULAR_QUEUE_HPP */