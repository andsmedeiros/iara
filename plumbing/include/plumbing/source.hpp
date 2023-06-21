#ifndef PLUMBING_SOURCE_HPP
#define PLUMBING_SOURCE_HPP

#include <vector>
#include <mutex>
#include <type_traits>
#include <utility>
#include <fuss.hpp>

namespace plumbing {
    
namespace messages {
    namespace source {
        template<class T>
        struct data_available : public fuss::message<T> {  };
    }
}

template<class T> class sink;
    
template<class T>
class source : private fuss::shouter<messages::source::data_available<T>> {
    template<class> friend class sink;
public:
    using type_out = T;
    
    virtual ~source() = default;
    
    virtual void produce(const T &data) {
        this->template shout<messages::source::data_available<T>>(data);
    }
    
    template<class T_collection, class = decltype(std::begin(std::declval<T_collection>()))>
    void produce(T_collection &&data) {
        for(auto &&datum : data) {
            produce(std::forward<decltype(datum)>(datum));
        }
    }
    
    virtual void pipe_to(sink<T> &sink) {
        sink.pipe_from(*this);
    }
    
    
    template<class T_sink>
    T_sink &operator>>(T_sink &sink) {
        sink.pipe_from(*this);
        return sink;
    }
};

    
} /* namespace plumbing */

#endif /* PLUMBING_SOURCE_HPP */