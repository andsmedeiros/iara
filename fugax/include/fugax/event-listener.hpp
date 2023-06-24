/**
 * @file fugax/include/fugax/event-listener.hpp
 * @brief Contains the definition of event listeners
 * @author André Medeiros
 * @date 22/06/23
 * @copyright 2023 (C) André Medeiros
**/

#ifndef FUGAX_EVENT_LISTENER_HPP
#define FUGAX_EVENT_LISTENER_HPP

#include <memory>
#include "event.hpp"

namespace fugax {

/**
 * @brief An event listener is a handle that can be used to cancel an event,
 * given it hasn't been executed yet
 */
using event_listener = std::weak_ptr<event>;

} /* namespace fugax */

#endif /* FUGAX_EVENT_LISTENER_HPP */
