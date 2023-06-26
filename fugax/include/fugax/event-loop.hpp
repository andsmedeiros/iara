/**
 * @file fugax/include/fugax/event-loop.hpp
 * @brief Contains definitions for the event loop and auxiliary constructs
 * @author André Medeiros
 * @date 22/06/23
 * @copyright 2023 (C) André Medeiros
**/

#ifndef FUGAX_EVENT_LOOP_HPP
#define FUGAX_EVENT_LOOP_HPP

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <config/fugax.hpp>
#include <juro/promise.hpp>
#include <juro/compose/race.hpp>
#include <utils/types.hpp>
#include "event.hpp"
#include "event-listener.hpp"
#include "event-guard.hpp"

namespace fugax {
using namespace config::fugax;
using namespace utils::types;

/**
 * @brief Tag struct to indicate an asynchronous timeout has occurred
 */
struct timeout {  };

/**
 * @brief Represents all possible forms of scheduling an event in the
 * event loop
 */
enum class schedule_policy {
    /**
     * @brief Will execute the task as soon as possible
     */
     immediate,
     /**
      * @brief Will execute the task after some time
      */
    delayed,
    /**
     * @brief Will execute the task periodically, being the first execution
     * as soon as possible
     */
    recurring_immediate,
    /**
     * @brief Will execute the task periodically, being the first execution
     * after some time
     */
    recurring_delayed,
    /**
     * @brief Will execute the task **every** runloop, until it is cancelled
     */
    always
};

/**
 * @brief An event loop is an object that coordinates execution of tasks.
 * @details The event loop has the ability to receive functors and store
 * them in varying ways to be able to execute them later. It can schedule
 * both recurring and one-shot tasks for either semi-immediate or delayed
 * execution.
 * Also it contains conveniency functions that deal with promises in a
 * temporal perspective.
 */
class event_loop {
    /**
     * @brief Queues of events are stored internally as linked lists of
     * shared pointers to events. This allows us to efficiently enqueue
     * events and merge queues.
     * The choice for the shared pointer occurs because it allows for the
     * event lifetime to be safely detached from the lifetime of event
     * listeners spread across the application. This makes both disposing
     * of events from inside the event loop and attempting to cancel them
     * from outside the event loop safe.
     */
    using event_queue = std::list<std::shared_ptr<event>>;

    /**
     * @brief All events are stored in the timer map. It contains a
     * collection of entries associating a task to its due time. When
     * the due time arrives, the task gets executed and, optionally,
     * gets re-scheduled then.
     * The unsigned integer type used represent timepoints can be configured
     * via the macro `FUGAX_TIME_TYPE`.
     */
    using timer_map = std::map<time_type, event_queue>;

    /**
     * @brief A mutex to lock scheduling operations.
     * @attention If `std::mutex` is unavailable in your platform,
     * supply via configuration an object type that responds to `.lock()`
     * and `.unlock()` and can ensure proper mutual exclusion.
     * E.g.: in bare-metal platforms, provide the macro `FUGAX_MUTEX_TYPE`
     * a custom mutex object type that, when locked, saves interruption
     * state and disabled them and, when unlock, restores interruption
     * state.
     */
    mutex_type mutex;

    /**
     * @brief Stores scheduled events, indexed by their due times.
     */
    timer_map timers;

    /**
     * @brief Keeps current execution time. As this value is updated, events
     * get executed if it matches their due time.
     */
    time_type counter = 0;

public:
    /**
     * @brief Main management function. Inform the loop of time passing.
     * By giving an update on what time is it now, instructs the loop to
     * execute all due tasks.
     * @param now New value for the internal time counter
     */
    void process(time_type now);

    /**
     * @brief Schedules a task for immediate execution
     * @param functor The task functor
     * @return An event listener that can be used to cancel the event
     * @see `fugax::event_loop::schedule(delay, policy, functor)`
     */
    event_listener schedule(event_handler functor);

    /**
     * @brief Schedules a task for delayed execution
     * @param delay How many units of time to delay execution
     * @param functor The task functor
     * @return An event listener that can be used to cancel the event
     * @see `fugax::event_loop::schedule(delay, policy, functor)`
     */
    event_listener schedule(time_type delay, event_handler functor);

    /**
     * @brief Schedules a task for delayed (and optionally recurring) execution
     * @param delay How many units of time to delay execution; if `recurring`
     * is true, this determines the period between two successive calls.
     * @param recurring If true, this function will be executed periodically
     * @param functor The task functor
     * @return An event listener that can be used to cancel the event
     * @see `fugax::event_loop::schedule(delay, policy, functor)`
     */
    event_listener schedule(time_type delay, bool recurring, event_handler functor);

    /**
     * @brief Schedules a task for eventual execution according to a supplied policy
     * @details This is the most flexible scheduling function available. It takes
     * a `policy` parameter that yield different behaviours by the event loop:
     * - If policy is `immediate`, the functor is enqueued for single execution in
     *   the current time slot. The `delay` parameter is ignored.
     * - If policy is `delayed`, the functor is enqueued for single execution in a
     *   future time slot. The `delay` parameter determines how many units of time
     *   in the future to schedule this task.
     * - If policy is `recurring_immediate`, the functor is enqueued for execution
     *   in the current time slot and will be continuously rescheduled for execution
     *   in intervals defined by the `delay` parameter.
     * - If policy is `recurring_delayed`, the functor is enqueued for execution in
     *   a future time slot and will be continuously rescheduled for execution
     *   in intervals defined by the `delay` parameter.
     * - If policy is `always`, the functor will be executed on every call to
     *   `.process()`, a.k.a. runloop . This is useful to bridge asynchronous and
     *   synchronous behaviour, but will incur considerable overhead to the loop
     *   latency if used incautiously.
     * @param delay How many units of time to delay execution; depending on the
     * provided policy, this also determines the period between two successive calls
     * @param policy How this task is to be scheduled
     * @param functor The task functor
     * @return An event listener that can be used to cancel the event
     */
    event_listener schedule(time_type delay, schedule_policy policy, event_handler functor);

    /**
     * @brief Schedules a task for continuous execution: it will be invoked every
     * @param functor The task functor
     * @return An event listener that can be used to cancel the event
     * @see `fugax::event_loop::schedule(delay, policy, functor)`
     */
    event_listener always(event_handler functor);

    /**
     * @brief Creates a new promise that resolves after some time
     * @param delay The delay until the promise resolution
     * @return The new promise pointer
     */
    juro::promise_ptr<fugax::timeout> wait(time_type delay);

    /**
     * @brief Returns a mutable lambda that can be called multiple times,
     * but will only execute the provided functor after some time of the
     * last call
     * @details `debounce` will create a new lambda that, when called,
     * will schedule a new event containing the provided functor to
     * be executed after `delay`; however, if execution is already
     * pending from a previously call, it is instead rescheduled, so
     * the provided functor is actually only executed once `delay` has
     * passed without any calls to the functor.
     * @tparam T_args A parameter pack that defines what arguments the
     * lambda accepts and forwards to the functor
     * @tparam T_functor The type of the functor to be invoked when the
     * call is finally debounced
     * @param delay The time during which the lambda must not be called
     * for the provided functor to be invoked
     * @param functor The functor to be executed
     * @return A new lambda that can be called repeatedly with `T_args...&`
     */
    template<class ...T_args, class T_functor>
    auto debounce(time_type delay, T_functor &&functor) {
        return [
            this,
            delay,
            functor = std::forward<T_functor>(functor),
            guard = std::make_shared<event_guard>()
        ] (T_args ...args) {
            if(const auto ev = guard->get().lock()) {
                ev->reschedule(counter + delay);
            } else {
                *guard = schedule(
                    delay,
                    [&, args = std::tuple { std::move(args)... }] {
                        std::apply(functor, args);
                    }
                );
            }
        };
    }

    /**
     * @brief Returns a mutable lambda that can be called multiple times,
     * but will only execute the provided functor from time to time,
     * silently swallowing intermediary calls
     * @details `throttle` will create a new lambda that, when called,
     * will invoke the provided functor and schedule a new event to
     * notify it that `delay` has passed; until it is notified, any calls
     * to the lambda will be ignored
     * @tparam T_args A parameter pack that defines what arguments the
     * lambda accepts and forwards to the functor
     * @tparam T_functor The type of the functor to be invoked
     * @param delay The minimum interval between two successive calls; also
     * the time while the lambda remains "disarmed"
     * @param functor The functor to be executed
     * @return A new lambda that can be called repeatedly with `T_args...&`
     */
    template<class ...T_args, class T_functor>
    auto throttle(time_type delay, T_functor &&functor) {
        return [
            this,
            delay,
            functor = std::forward<T_functor>(functor),
            guard = std::make_shared<event_guard>(),
            armed = true
        ] (T_args ...args) mutable {
            if(armed) {
                armed = false;
                *guard = schedule(delay, [&] { armed = true; });
                functor(std::move(args)...);
            }
        };
    }

    /**
     * @brief Creates a new promise that resolves either when the provided
     * promise is resolved or when the requested delay has passed
     * @tparam T_value The type of the promise to race against
     * @param delay The maximum time to wait fot the task to complete
     * @param promise The promise representing the asynchronous task
     * @return A new race promise that represents the race
     */
    template<class T_value>
    inline auto timeout(time_type delay, const juro::promise_ptr<T_value> &promise) {
        return juro::race(promise, wait(delay));
    }

    /**
     * @brief Shortcut for creating a new promise and providing it to
     * fugax::event_loop::timeout(time_type, const juro_promise_ptr<T_value> &)
     * @tparam T_value The type of the promise being created
     * @tparam T_launcher The type launcher functor of the promise being created
     * @param delay Time limit to wait for the promise to resolve
     * @param launcher The launcher functor to be supplied to
     * juro::make_promise<T_value>
     * @return A new race promise
     * @see fugax::event_loop::timeout(time_type, const juro_promise_ptr<T_value> &)
     */
    template<class T_value, class T_launcher>
    inline auto timeout(time_type delay, const T_launcher &launcher) {
        return timeout<T_value>(delay, juro::make_promise<T_value>(launcher));
    }

private:
    /**
     * @brief Collects from the timer map all events that are due; time entries
     * with a value different than the current counter will be deleted from the map
     * @return All events whose scheduled time is less than or equal to the current
     * counter as an `event_queue`
     */
    event_queue get_due_timers(time_type) noexcept;
};

} /* namespace fugax */

#endif /* FUGAX_EVENT_LOOP_HPP */