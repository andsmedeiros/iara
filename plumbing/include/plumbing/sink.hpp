#ifndef PLUMBING_SINK_HPP
#define PLUMBING_SINK_HPP

#include <fuss.hpp>
#include "plumbing/source.hpp"
#include <utils/circular-queue.hpp>

namespace plumbing {

namespace messages {

namespace active_sink {
    struct request_data : public fuss::message<std::size_t> {  };
} /* namespace active_sink */
} /* namespace messages */

template<class T>
class sink {
    fuss::message_guard guard;

public:
    using type_in = T;
    
    virtual ~sink() = default;
    virtual void consume([[maybe_unused]] const T &data) {  }
    
    template<class T_collection, class = decltype(std::begin(std::declval<T_collection>()))>
    void consume(T_collection &&data) {
        for(auto &&datum : data) {
            consume(std::forward<decltype(datum)>(datum));
        }
    }
    
    template<class T_source>
    T_source &operator<<(T_source &source) {
        pipe_from(source);
        return source;
    }
    
    void virtual piped([[maybe_unused]] source<T> &source) {  }

    void pipe_from(source<T> &source) {
        guard = source.template listen<messages::source::data_available<T>>([this] (T data) {
            consume(std::move(data));
        });
        piped(source);
    }
    
};

template<class T>
class active_sink : 
    public sink<T>, 
    public fuss::shouter<messages::active_sink::request_data> {
    
public:
    void request_data(std::size_t count) {
        this->shout<messages::active_sink::request_data>(count);
    }
};

template<class T>
class buffered_sink : public virtual sink<T>{
    circular_queue<T> queue;
    std::size_t count;
    
public:
    using sink<T>::consume;
    
    void consume(const T &data) final {
        if(this->count > 0 && this->queue.is_empty()) {
            this->count--;
            this->requested_data(data);
        } else {
            this->queue.push(data);
        }
    }
    
protected:
    virtual void requested_data(const T &data) = 0;
    
    void next(std::size_t count) {
        for(this->count = count; this->count > 0; this->count--) {
            if(this->queue.is_empty()) break;
            
            const T &data = this->queue.pop();
            this->requested_data(data);
        }
    }
};

} /* namespace plumbing*/

#endif /* PLUMBING_SINK_HPP */