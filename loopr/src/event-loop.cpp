#include "loopr/event-loop.hpp"

namespace loopr {

event_listener event_loop::schedule(event_handler functor) {
    return schedule(0, schedule_policy::IMMEDIATE, std::move(functor));
}

event_listener event_loop::schedule(u32 delay, event_handler functor) {
    return schedule(delay, schedule_policy::DELAYED, std::move(functor));
}

event_listener event_loop::schedule(u32 delay, bool recurring, event_handler functor) {
    auto policy = recurring ?
        schedule_policy::RECURRING_DELAYED :
        schedule_policy::DELAYED;
    return schedule(delay, policy, std::move(functor));
}

event_listener event_loop::schedule(u32 delay, schedule_policy policy, event_handler functor) {
    std::lock_guard _ { mutex };

    u32 slot, interval;
    bool recurring;

    switch(policy) {
    case schedule_policy::IMMEDIATE:
        std::tie(slot, recurring, interval) = std::tuple { counter, false, 0 };
        break;
    case schedule_policy::DELAYED:
        std::tie(slot, recurring, interval) = std::tuple { counter + delay, false, 0 };
        break;
    case schedule_policy::RECURRING_IMMEDIATE:
        std::tie(slot, recurring, interval) = std::tuple { counter, true, delay };
        break;
    case schedule_policy::RECURRING_DELAYED:
        std::tie(slot, recurring, interval) = std::tuple { counter + delay, true, delay };
        break;
    case schedule_policy::ALWAYS:
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
    return schedule(0, schedule_policy::ALWAYS, std::move(functor));
}

void event_loop::process(std::uint32_t now) {
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

juro::promise_ptr<loopr::timeout> event_loop::wait(u32 delay) {
    return juro::make_promise<loopr::timeout>([&] (const auto &promise) {
        schedule(delay, [=] { promise->resolve(); });
    });
}


event_loop::event_queue event_loop::get_due_timers(std::uint32_t now) noexcept {
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

} /* namespace loopr */