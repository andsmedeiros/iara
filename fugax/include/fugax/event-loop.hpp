#ifndef FUGAX_EVENT_LOOP_HPP
#define FUGAX_EVENT_LOOP_HPP

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <juro/promise.hpp>
#include <juro/compose/race.hpp>
#include <utils/types.hpp>
#include "event.hpp"

namespace fugax {
using namespace utils::types;

using event_listener = std::weak_ptr<event>;

class event_guard {
    event_listener listener;

public:
    inline event_guard() noexcept = default;
    event_guard(event_guard &&) noexcept = default;
    event_guard(const event_guard &) = delete;

    inline event_guard(event_listener &&listener) noexcept :
        listener { std::move(listener) }
    {  }

    ~event_guard() noexcept { release(); }

    inline event_guard &operator=(event_guard &&other) noexcept {
        release();
        listener = std::move(other.listener);
        return *this;
    }

    event_guard  &operator=(const event_guard &other) = delete;

    inline void release() const noexcept {
        if(const auto target = listener.lock()) {
            target->cancel();
        }
    }

    inline constexpr event_listener const &get() const noexcept { return listener; }
};

struct timeout {  };

template<class T>
using timeout_result = std::variant<T, timeout>;

template<class T>
using timeout_promise_ptr = juro::promise_ptr<timeout_result<T>>;

enum class schedule_policy { 
    immediate,
    delayed,
    recurring_immediate,
    recurring_delayed,
    always
};

class event_loop {
    using event_queue = std::list<std::shared_ptr<event>>;
    using timer_map = std::map<std::uint32_t, event_queue>;
    
    timer_map timers;

    std::mutex mutex;
    u32 counter = 0;

public:
    void process(u32 now);
    
    event_listener schedule(event_handler functor);
    event_listener schedule(u32 delay, event_handler functor);
    event_listener schedule(u32 delay, bool recurring, event_handler functor);
    event_listener schedule(u32 delay, schedule_policy policy, event_handler functor);
    event_listener always(event_handler functor);


    juro::promise_ptr<fugax::timeout> wait(u32 delay);

    template<class ...T_args, class T_functor>
    auto debounce(u32 delay, T_functor &&functor) {
        return [
            this,
            delay,
            functor = std::forward<T_functor>(functor),
            guard = std::make_shared<event_guard>()
        ] (auto &&...args) {
            if(const auto ev = guard->get().lock()) {
                ev->reschedule(counter + delay);
            } else {
                *guard = schedule(
                    delay,
                    [&, args = std::tuple { args... }] {
                        std::apply(functor, args);
                    }
                );
            }
        };
    }

    template<class T_functor>
    auto throttle(u32 delay, T_functor &&functor) {
        return [
            this,
            delay,
            functor = std::forward<T_functor>(functor),
            guard = std::make_shared<event_guard>(),
            armed = true
        ] (auto &&...args) mutable {
            if(armed) {
                armed = false;
                *guard = schedule(delay, [&] { armed = true; });
                functor(std::forward<decltype(args)>(args)...);
            }
        };
    }

    template<class T_value>
    inline auto timeout(u32 delay, const juro::promise_ptr<T_value> &promise) {
        return juro::race(promise, wait(delay));
    }

    template<class T_value, class T_launcher>
    inline auto timeout(u32 delay, const T_launcher &launcher) {
        return timeout<T_value>(delay, juro::make_promise<T_value>(launcher));
    }

private:
    event_queue get_due_timers(std::uint32_t) noexcept;
};

} /* namespace fugax */

#endif /* FUGAX_EVENT_LOOP_HPP */