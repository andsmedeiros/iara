/**
 * @file fugax/include/fugax/event.hpp
 * @brief Definitions for events and auxiliary constructs
 * @author André Medeiros
 * @date 26/06/23
 * @copyright 2023 (C) André Medeiros
 */

#ifndef FUGAX_EVENT_HPP
#define FUGAX_EVENT_HPP

#include <atomic>
#include <memory>
#include <type_traits>
#include <config/fugax.hpp>

namespace fugax {
using namespace config::fugax;

class event;

/**
 * @brief Basic invocable interface, used for type-erasing event handler functors
 */
class invocable_interface {
public:
    /**
     * @brief Invoke action; should execute the wrapped functor
     */
    virtual void invoke(event &) = 0;
    virtual ~invocable_interface() noexcept = default;
};

/**
 * @brief A concrete implementation of the invocable interface that wraps
 * a determined type of functor
 * @tparam T_functor The type of the functor that is wrapped by this invocable
 */
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
    /**
     * @brief The functor object wrapped by this invocable; when the invocable's
     * `.invoke()` member function is called, this functor will be executed
     */
    T_functor functor;

    /**
     * @brief Constructs a new invocable object from the supplied functor
     * @tparam T_func The type of the functor being wrapped
     * @param func The functor object to be stored inside the invocable
     */
    template<class T_func>
    inline invocable(T_func &&func) noexcept : 
        functor { std::forward<T_func>(func) } 
    { }

    /**
     * @brief Copy constructor is deleted; invocables are move-only
     */
    invocable(const invocable &) = delete;

    /**
     * @brief Move constructor is defaulted and is noexcept if the wrapped
     * functor type is nothrow move constructible;
     */
    invocable(invocable &&)
        noexcept(std::is_nothrow_move_constructible_v<T_functor>) = default;

    /**
     * @brief Invokes the wrapped functor, passing the associated event along if
     * it accepts parameters
     * @param ev The fired event that originated this invocable call
     */
    virtual void invoke(event &ev) override {
        if constexpr(std::is_invocable_v<T_functor>) {
            functor();
        } else {
            functor(ev);
        }
    }
};

/**
 * @brief A type-erased container for event handlers
 */
class event_handler {
    /**
     * @brief The type-erased pointer to the invocable instance
     */
    std::unique_ptr<invocable_interface> handler;

public:
    /**
     * @brief Constructs a new event handler from a given functor
     * @tparam T_functor The type of the functor referenced by this event handler
     * @param functor The functor that gets executed when the handler is called
     */
    template<class T_functor>
    inline event_handler(T_functor &&functor) :
        handler { 
            std::make_unique<invocable<T_functor>>(std::forward<T_functor>(functor))
        }
    {  }

    /**
     * @brief Calls this event handler
     * @param ev The fired event that originated this handler call
     */
    void operator()(event &ev) const;
};

class event_loop;

/**
 * @brief An event represents an asynchronous task scheduled in the event loop
 */
class event {
    friend event_loop;

    /**
     * @brief The event handler to be called when this event's due time arrives
     */
    const event_handler handler;

    /**
     * @brief For recurring events, this field stores how many time units must
     * pass between two successive activations of this event; ignored for
     * non-recurring events
     */
    const time_type interval;

    /**
     * @brief The time value when execution is intended to occur; this gets updated
     * when an event is rescheduled, so a mismatch between the timer map key under
     * which this event is stored and this value indicates that the event must be
     * moved to a later slot instead of fired
     */
    std::atomic<time_type> due_time;

    /**
     * @brief A flag that indicates whether this event should be fired just once or
     * multiple times
     */
    const bool recurring;

    /**
     * @brief A flag that indicates if an event has been cancelled, what will cause it
     * to not be fired anymore by the event loop and be destroyed when its due time
     * arrives
     */
    std::atomic<bool> cancelled = false;

    /**
     * @brief Fires this event, invoking its corresponding handler
     */
    void fire();

public:

    /**
     * @brief Constructs a new event
     * @param handler The handler to be called when this event is due
     * @param interval The interval between two successive activations of this event;
     * is ignored unless the `recurring` parameter is true
     * @param due_time This event's due time
     * @param recurring Whether this event is recurring or one-shot
     */
    event(event_handler &&handler, time_type interval, time_type due_time, bool recurring);

    /**
     * @brief Cancels this event, preventing future execution; it will also cause the
     * event to be destroyed by the event loop eventually
     */
    void cancel() noexcept;

    /**
     * @brief Reschedules this event to be run in a different time then what was initially
     * set; when the original time value compares equal to the counter value; the event
     * will be relocated around the timer map
     * @param time_point This event's new due time
     */
    void reschedule(time_type time_point) noexcept;

    /**
     * @brief Returns whether this event has been cancelled or not
     * @return Whether the internal `cancelled` flag is set
     */
    inline bool is_cancelled() const noexcept { return cancelled; }
};

} /* namespace fugax */

#endif /* FUGAX_EVENT_HPP */