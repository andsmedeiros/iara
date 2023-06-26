/**
 * @file fugax/src/event-guard.cpp
 * @brief Implementation of non-templated event guard functions
 * @author André Medeiros
 * @date 22/06/23
 * @copyright 2023 (C) André Medeiros
**/

#include "fugax/event-guard.hpp"

namespace fugax {

event_guard &event_guard::operator=(event_guard &&other) noexcept {
    release();
    listener = std::move(other.listener);
    return *this;
}

void event_guard::release() const noexcept {
    if(const auto target = listener.lock()) {
        target->cancel();
    }
}

} /* namespace fugax */