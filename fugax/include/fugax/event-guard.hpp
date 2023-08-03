/**
 * @file fugax/include/fugax/event-guard.hpp
 * @brief Contains definition of event guards, a RAII-enabled container for
 * event listeners
 * @author André Medeiros
 * @date 22/06/23
 * @copyright 2023 (C) André Medeiros
**/

#ifndef IARA_EVENT_GUARD_HPP
#define IARA_EVENT_GUARD_HPP

#include "event-listener.hpp"

namespace fugax {

/**
 * @brief An event guard is a RAII-style container for an event listener.
 * It has a converting constructor and can be directly initialised from
 * an event listener. Upon destruction, it will attempt to cancel the
 * event
 */
class event_guard {
    /**
     * @brief The listener (i.e. a weak pointer) of the event; upon
     * destruction, the guard will attempt to release its ownership
     * over this
     * @see `fugax::event_guard::release`
     */
    event_listener listener;

public:

    /**
     * @brief Default constructor; yields an empty event guard
     */
    inline constexpr event_guard() noexcept = default;

    /**
     * @brief Move constructor; empties the other guard and assumes
     * ownership of its listener
     */
    inline event_guard(event_guard &&) noexcept = default;

    /**
     * @brief Copy constructor is deleted; event guards are move-only
     */
    event_guard(const event_guard &) = delete;

    /**
     * @brief Converting constructor for event listeners
     * @param listener The event listener to store in the guard
     */
    inline event_guard(event_listener &&listener) noexcept :
        listener { std::move(listener) }
    {  }

    /**
     * @brief Converting constructor for event listeners
     * @param listener The event listener to store in the guard
     */
    inline event_guard(const event_listener &listener) noexcept :
        listener { listener }
    {  }

    /**
     * @brief Upon destruction, the guard attempts to release its ownership
     * by calling `.release()`
     * @see `fugax::event_guard::release`
     */
    inline ~event_guard() noexcept { release(); }

    /**
     * @brief Move-assignment operator; reassigns this guard by providing it
     * another guard's listener; if this guard owns a listener already, it will
     * attempt to release it first
     * @param other Another event guard (that might have been construct via
     * conversion construction) whose listener will be moved into this guard;
     * after assignment, it will be empty
     * @return A reference to `this`
     */
    event_guard &operator=(event_guard &&other) noexcept;

    /**
     * @brief Copy-assignment is deleted because `event_guard` is a move-only
     * type
     */
    event_guard &operator=(const event_guard &other) = delete;

    /**
     * @brief Attempts to release the holden listener by acquiring its
     * shared pointer, and, if succeeded, cancels the event; if locking
     * the listener fails, does nothing
     */
    void release() const noexcept;

    /**
     * @brief Gets a const reference to the contained event listener
     * @return A read-only reference to the contained listener
     */
    inline constexpr event_listener const &get() const noexcept { return listener; }
};

} /* namespace fugax */

#endif /* IARA_EVENT_GUARD_HPP */
