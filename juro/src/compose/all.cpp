#include "juro/promise.hpp"
#include "juro/compose/all.hpp"

namespace juro::compose {

void void_settler(const promise_ptr<void> &promise, const promise_ptr<void> &all_promise, const std::shared_ptr<std::size_t> &counter) {
    promise->then([counter, all_promise] {
        if(--(*counter) == 0 && all_promise->is_pending()) {
            all_promise->resolve();
        }
    }, [all_promise] (std::exception_ptr error) {
        if(all_promise->is_pending()) {
            all_promise->reject(error);
        }
    });
}

} /* namespace juro::compose */