#ifndef PLUMBING_PROXY_HPP
#define PLUMBING_PROXY_HPP

#include "duplex.hpp"

namespace plumbing {
    
namespace proxy {

    
template<class T>
struct source : public virtual stream::source<T> {
    source(stream::source<T> &target) : stream::source<T>() {  
        target.listen<messages::source::data_available<T>>([this](const T &data) {
            this->produce(data);
        });
    }  
};

template<class T>
struct sink : public virtual stream::sink<T> {
    stream::sink<T> &target;
    
    using stream::sink<T>::consume;
    
    sink(stream::sink<T> &target) : stream::sink<T>(), target(target) {  }
    
    void consume(const T &data) {
        this->target.consume(data);
    }
};

template<class T_in, class T_out = T_in>
struct duplex : 
    public stream::duplex<T_in, T_out>, public sink<T_in>, public source<T_out> {
    
    template<class T_target>
    duplex(T_target &target) :
        sink<T_in>(target), source<T_out>(target) {  }
    
    template<class T_target_in, class T_target_out>
    duplex(T_target_in &target_in, T_target_out &target_out) :
        sink<T_in>(target_in), source<T_out>(target_out) {  }
};
    

} /* namespace proxy */
    
} /* namespace plumbing */

#endif /* PLUMBING_PROXY_HPP */