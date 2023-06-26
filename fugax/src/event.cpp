/**
 * @file fugax/src/event.cpp
 * @brief Implementation of non-templated event functions
 * @author André Medeiros
 * @date 26/06/23
 * @copyright 2023 (C) André Medeiros
 */

 #include "fugax/event.hpp"

namespace fugax {

void event_handler::operator()(event &ev) const { handler->invoke(ev); }

event::event(event_handler &&handler, time_type interval, time_type due_time, bool recurring) :
    handler { std::forward<event_handler &&>(handler) },
    interval { interval },
    due_time { due_time },
    recurring { recurring }
{  }

void event::fire() { handler(*this); }
void event::cancel() noexcept { cancelled = true; }
void event::reschedule(time_type time_point) noexcept { due_time = time_point; }

} /* namespace fugax */