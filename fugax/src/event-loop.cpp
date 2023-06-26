/**
 * @file fugax/src/event-loop.cpp
 * @brief Implementation of non-templated event loop functions
 * @author André Medeiros
 * @date 26/06/23
 * @copyright 2023 (C) André Medeiros
 */

#include "fugax/event-loop.hpp"

namespace fugax {

event_listener event_loop::schedule(event_handler functor) {
    return schedule(0, schedule_policy::immediate, std::move(functor));
}

event_listener event_loop::schedule(time_type delay, event_handler functor) {
    return schedule(delay, schedule_policy::delayed, std::move(functor));
}

event_listener event_loop::schedule(time_type delay, bool recurring, event_handler functor) {
    auto policy = recurring ?
        schedule_policy::recurring_delayed :
        schedule_policy::delayed;
    return schedule(delay, policy, std::move(functor));
}

event_listener event_loop::schedule(time_type delay, schedule_policy policy, event_handler functor) {
    std::lock_guard _ { mutex };

    time_type slot, interval;
    bool recurring;

    switch(policy) {
    case schedule_policy::immediate:
        std::tie(slot, recurring, interval) = std::tuple { counter, false, 0 };
        break;
    case schedule_policy::delayed:
        std::tie(slot, recurring, interval) = std::tuple { counter + delay, false, 0 };
        break;
    case schedule_policy::recurring_immediate:
        std::tie(slot, recurring, interval) = std::tuple { counter, true, delay };
        break;
    case schedule_policy::recurring_delayed:
        std::tie(slot, recurring, interval) = std::tuple { counter + delay, true, delay };
        break;
    case schedule_policy::always:
        std::tie(slot, recurring, interval) = std::tuple { counter, true, 0 };
        break;
    default:
        return {  };
    }

    return timers[slot].emplace_back(
        std::make_shared<event>(std::move(functor), recurring, delay, slot)
    );
}

event_listener event_loop::always(event_handler functor) {
    return schedule(0, schedule_policy::always, std::move(functor));
}

void event_loop::process(time_type now) {
    auto queue = get_due_timers(now);

    auto entry = queue.begin();
    while(entry != queue.end()) {
        const auto removing = entry++;
        const auto &event = *removing;

        if(event->cancelled) continue;

        if(event->due_time <= now) { // Event is due to be fired
            event->fire();

            if(event->recurring) {
                std::lock_guard _ { mutex };
                auto &target = timers[now + event->interval];
                target.splice(target.end(), queue, removing);
            }
        }
        else { // Event has been rescheduled
            std::lock_guard _ { mutex };
            auto &target = timers[event->due_time];
            target.splice(target.end(), queue, removing);
        }
    }

    counter = now;
}

juro::promise_ptr<fugax::timeout> event_loop::wait(time_type delay) {
    return juro::make_promise<fugax::timeout>([&] (const auto &promise) {
        schedule(delay, [=] { promise->resolve(); });
    });
}


event_loop::event_queue event_loop::get_due_timers(time_type now) noexcept {
    event_queue queue;
    std::lock_guard _ { mutex };

    auto entry = timers.begin();
    while(entry != timers.end()) {
        const auto removing = entry++;
        auto & [ time_point, events ] = *removing;
        if(time_point > now) break;

        queue.splice(queue.end(), events);
        if(time_point != now) {
            timers.erase(removing);
        }
    }

    return queue;
}

} /* namespace fugax */
