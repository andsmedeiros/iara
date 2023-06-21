#include "juro/promise.hpp"

namespace juro {

promise_interface::promise_interface(promise_state state) noexcept :
    state { state }
{  }

void promise_interface::set_settle_handler(std::function<void()> &&handler) noexcept {
    on_settle = std::move(handler);
    if(is_settled()) {
        on_settle();
    }
}

void promise_interface::resolved() noexcept {
    state = promise_state::RESOLVED;
    if(on_settle) {
        on_settle();
    }
}

void promise_interface::rejected() {
    state = promise_state::REJECTED;
    if(on_settle) {
        on_settle();
    } else {
        throw promise_error { "Unhandled promise rejection" };
    }
}

} /* namespace juro */