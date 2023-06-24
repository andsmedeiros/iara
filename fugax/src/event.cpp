#include "fugax/event.hpp"

namespace fugax {

void event_handler::operator()(event &ev) const { handler->invoke(ev); }

event::event(event_handler &&handler, bool recurring, time_type interval, time_type due_time) :
    interval { interval },
    handler { std::forward<event_handler &&>(handler) },
    recurring { recurring },
    due_time { due_time }
{  }

void event::fire() { handler(*this); }
void event::cancel() noexcept { cancelled = true; }
void event::reschedule(time_type time_point) noexcept { due_time = time_point; }

} /* namespace fugax */