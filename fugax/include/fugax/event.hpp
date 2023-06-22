#ifndef FUGAX_EVENT_HPP
#define FUGAX_EVENT_HPP

#include <utils/types.hpp>
#include <memory>
#include <type_traits>

namespace fugax {
using namespace utils::types;

class event;

class invocable_interface {
public:
    virtual void invoke(event &) = 0;
    virtual ~invocable_interface() = default;
};

template<class T_functor>
class invocable : public invocable_interface {
    static_assert(
        std::disjunction_v<
            std::is_invocable<T_functor, event &>,
            std::is_invocable<T_functor>
        >, 
        "An event handler functor must accept one event& parameter " \
        "or no parameter at all."
    );

public:
    T_functor functor;

    template<class T_func>
    inline invocable(T_func &&func) noexcept : 
        functor { std::forward<T_func>(func) } 
    { }
    invocable(const invocable &) = default;
    invocable(invocable &&) = default;

    virtual void invoke(event &ev) override {
        if constexpr(std::is_invocable_v<T_functor>) {
            functor();
        } else {
            functor(ev);
        }
    }
};

class event_handler {
    std::unique_ptr<invocable_interface> handler;

public:
    template<class T_functor>
    inline event_handler(T_functor &&functor) :
        handler { 
            std::make_unique<invocable<T_functor>>(std::forward<T_functor>(functor))
        }
    {  }

    void operator()(event &ev) const;
};

class event_loop;

class event {
    friend event_loop;

    const u32 interval;
    const event_handler handler;
    const bool recurring;
    u32 due_time;
    bool cancelled = false;

    void fire();

public:
    event(event_handler &&handler, bool recurring, u32 interval, u32 due_time);
    void cancel() noexcept;
    void reschedule(u32 time_point) noexcept;
};

} /* namespace fugax */

#endif /* FUGAX_EVENT_HPP */