#ifndef PLUMBING_DUPLEX_HPP
#define PLUMBING_DUPLEX_HPP

#include "source.hpp"
#include "sink.hpp"
#include <cstdint>

namespace plumbing {
    
template<class T_in, class T_out = T_in>
struct duplex : public virtual sink<T_in>, public virtual source<T_out> { 
    using type_in = T_in;
    using type_out = T_out;
};

template<class T_in, class T_out>
class transform : public duplex<T_in, T_out> {
    
    using transform_function = std::function<const T_out &(const T_in &)>;
    transform_function apply;
    
public:
    transform(transform_function apply) : apply(apply) {  }
    
    void consume(const T_in &data) final {
        this->produce(this->apply(data));
    }
};

template<class T>
class buffer : 
    public duplex<T>,
    public buffered_sink<T> {
    
protected:
    void pipe_to(sink<T> &target) final {
        try {
            auto &active = dynamic_cast<active_sink<T> &>(target);
            active.listen<messages::active_sink::request_data>(
                [this](const std::size_t count) {
                   this->next(count); 
                }
            );
        } catch(std::bad_cast &error) {  }
        source<T>::pipe_to(target);
    }
    
public:
    void requested_data(const T &data) {
        this->produce(data);
    }
};


template<class T>
class splitter : public duplex<std::vector<T>, T> {
    
    void consume(const std::vector<T> &vector) override {
        for(const T &element : vector) {
            this->produce(element);
        }
    }
};


class string_splitter : public duplex<std::string, std::uint8_t> {
    
    void consume(const std::string &vector) override {
        for(const auto &element : vector) {
            this->produce(element);
        }
    }
};

    
} /* namespace plumbing */

#endif /* PLUMBING_DUPLEX_HPP */