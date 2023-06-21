#ifndef PLUMBING_BOX_HPP
#define PLUMBING_BOX_HPP

#include "source.hpp"
#include "sink.hpp"
#include <tuple>


namespace plumbing {
namespace box {
namespace {
    
    template<class T_first, class T_last>
    T_last *pipe_many(T_first *first, T_last *last) {
        first->pipe_to(*last);
        return last;
    }
    
    template<class T_first, class T_next, class ... T_rest, class T_last>
    T_last *pipe_many(T_first *first, T_next *next, T_rest * ... rest, T_last *last) {
        first->pipe_to(*next);
        return pipe_many(next, rest ..., last);
    }    
    
    template <class ... T_args>
    class box {
    protected:
        std::tuple<T_args * ... > segments;

    public:
        virtual ~box() = default;
        box(T_args * ... args) : segments(args ...) { 
            pipe_many(args ...); 
        }
    };
    
    template<class ... T_args>
    class first {
        using tuple = std::tuple<T_args ...>;
    public:
        using type = typename std::tuple_element<0, tuple>::type::type_in;
    };

    template<class ... T_args>
    class last {
        using tuple = std::tuple<T_args ...>;
        using pos = std::tuple_size<tuple>;
    public:
        using type = typename std::tuple_element<pos::value - 1, tuple>::type::type_out;
    };
    
    template<class ... T_args>
    class source : 
        public stream::source<typename last<T_args ...>::type>,
        public virtual box<T_args ...> {

    public:     
        source(T_args * ... segments) : box<T_args ...>(segments ...) {  }
        
    protected:
        using source_type = typename last<T_args ...>::type;
        
        void pipe_to(sink<source_type> &target) final {
            auto constexpr last_pos = 
                std::tuple_size<std::tuple<T_args * ...>>::value - 1;
            *std::get<last_pos>(this->segments) >> target;
        }
    };

    template <class ... T_args>
    class sink : 
        public stream::sink<typename first<T_args ...>::type>,
        public virtual box<T_args ...> {

        void consume(const typename first<T_args ...>::type &data) final {
            std::get<0>(this->segments)->consume(data);
        }

    public:     
        sink(T_args * ... segments) : box<T_args ...>(segments ...) {  }


    };

    template<class ... T_args>
    class duplex : 
        public stream::box::sink<T_args ...>, 
        public stream::box::source<T_args ...> {

        duplex(T_args * ... segments) : 
            stream::box::source<T_args ...>(segments ...) {  }
    };
} /* anonymous namespace */

template<class ... T_args>
source<T_args ...> *make_source(T_args * ... args) {
    return new source<T_args ...>(args ...);
}


} /* namespace box */

} /* namespace plumbing */


#endif /* PLUMBING_BOX_HPP */