# Juro

> *Eu juro*
>
> *Por mim mesmo, por deus, por meus pais*
>
> *Vou te amar~~~*


**Juro** is a modern implementation of Javascript promises in C++17. It aims to 
provide a building block with which asynchronous agents can easily interact, while 
maintaining a readable and safe interface.  

## Table of contents

<!-- TOC -->
* [Juro](#juro)
  * [Table of contents](#table-of-contents)
  * [Disclaimer](#disclaimer)
  * [Basic example](#basic-example)
  * [Promises (and a little of Javascript)](#promises-and-a-little-of-javascript)
    * [An introduction to Javascript promises](#an-introduction-to-javascript-promises)
    * [Juro: an approximation of JS promises that leverages C++ facilities](#juro-an-approximation-of-js-promises-that-leverages-c-facilities)
      * [Event loop?](#event-loop)
    * [`juro::promise` and `juro::promise_ptr`](#juropromise-and-juropromiseptr)
    * [Promise factories](#promise-factories)
    * [Promise settling](#promise-settling)
      * [Handling resolution](#handling-resolution)
      * [Handling rejection](#handling-rejection)
      * [Handling resolution and rejection at once](#handling-resolution-and-rejection-at-once)
      * [Handling after settling](#handling-after-settling)
    * [Promise chaining](#promise-chaining)
      * [Synchronous chaining](#synchronous-chaining)
      * [Asynchronous chaining](#asynchronous-chaining)
      * [Chained promise type](#chained-promise-type)
    * [Promise composition](#promise-composition)
      * [`juro::all()`](#juroall)
      * [`juro::race()`](#jurorace)
    * [Promise lifetime and memory management](#promise-lifetime-and-memory-management)
  * [Roadmap](#roadmap)
<!-- TOC -->

## Disclaimer

This is a WIP. Tests are not comprehensive, documentation is lacking and the API is still being 
defined. It is however feature rich and very functional.

## Basic example

```C++
#include <juro/promise.hpp>

juro::make_promise<int>([] (auto &promise) {
    start_some_async_operation(promise);
})
->then([] (int value) {
    std::cout << "Got value" << value << std::endl;
});

// In `start_some_async_operation`, some time later:
promise->resolve(10);
```

## Promises (and a little of Javascript)

### An introduction to Javascript promises

(If you are comfortable with the concept of a JS Promise, feel free to skip this).

A promise of a value is an object that can be created even when that value is still not available
and can, once the value is available, hold and provide it to subsequent steps that depend on that 
value.

For example, in Javascript:

```JS
new Promise(resolve => setTimeout(() => resolve(327), 100))
```

A new `Promise` object is created (via `operator new`) and its constructor is invoked with a
function parameter. This function takes a `resolve` functor as parameter that can be used to
*settle* the promise as *resolved*. The function then proceeds to schedule the call of `resolve` 
with a value `327` 100 milliseconds into the future.

Now, suppose we didn't know with which value the promise would be resolved. Nonetheless, we could
still define what should be done with whichever value the promise would be resolved.

```JS
function asyncNumberGenerator(callback) {
    setTimeout(() => callback(Math.random()), 1000)
}

new Promise(resolve => asyncNumberGenerator(resolve))
    .then(number => console.log(`Got number: ${number}`))
```

The function `asyncNumberGenerator` invokes the provided `callback` function with a random number 
one second after called. Because it is provided the `resolve` functor, it will resolve the
promise with a random value.

The call to `.then()` sets a handler that gets called when the promise is resolved. Once a value 
is available, the handler will be invoked with the value as parameter. In this case, 
`'Got number: NUMBER'` will be printed, for whichever `NUMBER` is received from the asynchronous
number generator.

Besides coordinating asynchronous work, promises can also be used to set asynchronous error 
recovery paths. For example, the following function takes the path of a file as parameter and 
prints its content on the console:

```JS
import { readFile } from 'node:fs'

function printFileContents(path) {
    new Promise(resolve => readFile(path, (err, data) => resolve(data)))
        .then(contents => console.log(`Contents of file "${file}":`, contents))
}

printFileContents('path/to/file.txt')
```

If an invalid path is provided, the function fails to abort the process. Instead, it will output
the same as if the file existed but was empty. 
To clearly indicate that an error occurred and offer possibility of error handling, the promise 
can be *rejected*:

```JS
// ...
new Promise((resolve, reject) => readFile(path, (err, data) => {
    if(err) { reject(err) }
    else { resolve(data) }
}))
// ...
```

Now, when an invalid path is passed to out function, an error will be thrown. To handle it, a
rejection handler can be attached:

```JS
// ...
    .then(
        contents => console.log(`Contents of file "${file}":`, contents),
        error => console.error(`Failed to open file "${file}":`, error)
    )
// ...
```

This way, the error can be caught and a suitable error message will be printed to `stderr`.

At last, promises can be chained. All calls to `.then()` (and its cousin functions `.catch()` 
and `.finally()`) return another promise that can also be `.then`ed.

```JS
function resolvesInOneSecond() {
    return new Promise(resolve => setTimeout(resolve, 1000))
}

resolvesInOneSecond()
    .then(() => {
        console.log("A")
        return resolvesInOneSecond()
    })
    .then(() => {
        console.log("B")
        return resolvesInOneSecond()
    })
    .then(() => {
        console.log("C")
    })
```

As each call to `resolvesInOneSecond()` returns a new promise, each handler attached by `.then()`
will only be called when the previous promise is settled. As such, `ABC` is printed with a 
one-second-delay between each character.

**See also:** [MDN documentation on promises](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise)

### Juro: an approximation of JS promises that leverages C++ facilities

C++ and Javascript are very different languages. Many features that make implementing promises
in Javascript very easy, such as weak typing, dynamic objects with counted references by default,
lack of multithreading etc. are not as immediately available to C++ programs or do not live up to
design, performance and safety expectations of C++ developers.

Juro attempts to bridge these two words. It provides a way to represent asynchronous operations
as self-contained objects that can be composed irrespective of time, while also leveraging C++ 
idioms, STL containers and compile-time type safety.

Juro's API is inspired and modelled after Javascript's one.

#### Event loop?

Javascript is a single threaded environment that operates around an event loop. Promises are,
therefore, guaranteed to be scheduled and ran on the same single thread. This is not true for
C++ programs, at least not necessarily. 

To avoid concurrency issues, this guide will presume there is available an event loop object
with capabilities and API similar to JS's one and that every instruction run will originate in 
it:

```C++
#include <functional>

class event_loop {
public:
    using task = std::function<void()>;
    void schedule(task, unsigned int delay = 0);
    
    // other implementation details
};

event_loop loop;
```

### `juro::promise` and `juro::promise_ptr`

All promises in Juro are represented by instances of the `juro::promise<T>` template. However,
because asynchronous operations must be accessible at a later time for settling, they are
dynamically allocated and managed by shared pointers -- `std::shared_ptr<juro::promise<T>>`.

To avoid the cumbersome template instantiating, there is a conveniency alias 
`juro::promise_ptr<T>` that should be preferred and that will be used throughout this guide.

### Promise factories

All promises are meant to be created via one of the available promise factories.

The most immediately obvious factory mimics Javascript's promise constructor:

```C++
template<class T, class T_launcher>
juro::promise_ptr<T> juro::make_promise(T_launcher &&launcher);
```

`juro::make_promise` takes a single launcher functor argument, that gets a fresh 
`const promise_ptr<T> &` as parameter. This handle allows for an asynchronous operation to settle
the promise later on. A copy of the handle is also returned:

```C++
juro::promise_ptr<std::string> promise =
    juro::make_promise<std::string>([] (const juro::promise_ptr<std::string> &promise) {
        store_handle_and_settle_later_on(promise);
    });

// or the much more readable form:
auto promise = juro::make_promise<std::string>([] (auto &promise) {
   store_handle_and_settle_later_on(promise); 
});
```

> **Warning!** Unlike Javascript promise constructors, the launcher functor in 
> `juro::make_promise` is executed synchronously.

Promises can also be explicitly created in any state:

```C++
// juro::promise_ptr<bool>
auto pending_promise = juro::make_pending<bool>();

// juro::promise_ptr<int>
auto resolved_promise = juro::make_resolved<int>(-300); 

// juro::promise_ptr<std::string>
auto rejected_promise = 
    juro::make_rejected<std::string>(std::runtime_error { "Runtime error" });
```

When the template parameter is excluded from any factory function, they default to creating 
`juro::promise<void>`.

### Promise settling

Unlike Javascript promises that can only be settled through the `resolve` and `reject` functors
provided by the promise constructor, `juro::promise`s can be settled by any party that owns a
copy of its `juro::promise_ptr`.

```C++
juro::make_promise([] (auto &promise) {
  loop.schedule([promise] { promise->resolve(); }, 1000);
  
  // or:
  
  loop.schedule([promise] { promise->reject(); }, 1000);
});
```

If the promise type `T` is non-void and default-constructible, calling `resolve` without parameters
results in the promise being resolved with a default-constructed value of type `T`, so the following
resolutions yield the same:

```C++
juro::make_promise<int>([] (auto &promise) {
    promise->resolve();
    
    // same as:
    
    promise->resolve(0);
});
```

Similarly, if `reject` is called without parameters, a `juro::promise_error` is constructed by
default and serves as the promise rejection reason:

```C++
  juro::make_promise<std::string>([] (auto &promise) {
      promise->reject();
      
      // same as:
      
      promise->reject(juro::promise_error { "Promise was rejected" });
  })
```

Once a promise is settled, it retains its state and value forever; attempting to resettle it 
will trigger an exception.

> **Warning!** Promise resolution and rejection are two very different things. They represent
> ordinary and exceptional execution paths, respectively, and thus are subject to their natural
> consequences:
> - Resolution is modelled after ordinary execution. Any value resolved is stored inside the
>   promise object, so it doesn't need any dynamic allocation.
> - Rejection is modelled after exception handling. Taking in account that basically anything 
>   can be thrown in C++, in order to be able to store an exception and later provide it for 
>   rejection handlers, whatever is thrown gets stored behind an `std::exception_ptr`, which is a
>   type-erased pointer of an exception object that has unspecified storage.
>   
>   With an `std::exception_ptr` and standard functions such as `std::rethrow_exception`, the 
>   programmer can disambiguate thrown values in a very elegant manner.
> 
> Because proper exception handling involves realtime type information, type erasure, stack 
> unwinding and possibly dynamic memory allocation, it incurs a considerable amount of work
> upon that of ordinary execution, where simple value copies and moves are performed.
> 
> Keeping exception handling for exceptional situations is usually the wisest choice. `.reject()`
> with care.

#### Handling resolution

To handle a resolved value, a resolve handler can be installed via `.then()`:

```C++
juro::make_promise<int>([] (auto &promise) {
    loop.schedule([promise] { promise->resolve(100); }, 1000);
})
->then([] (int value) {
    std::cout << "Got value " << value << std::endl;
});
```

If a promise is of type void (`juro::promise<void>`), its resolve handler must not take any
parameter.
```C++
juro::make_promise<void>([] (auto &promise) {
    loop.schedule([promise] { promise->resolve(); }, 1000);
})
->then([] {
    std::cout << "Promise resolved!" << std::endl;
});
```

The functor passed to `.then()` is statically checked, so there is no risk of type mismatch 
between resolution and handling. Also, no type erasing or dynamic allocation is needed to store 
the value; it lives right in the `juro::promise`. There is only a single `std::function` type 
erased object involved in the promise chain; once the chain is constructed, no more memory is
necessary.

#### Handling rejection

The semantics of promise rejection in Javascript imply that any exception thrown inside an
asynchronous task gets translated as the rejection of the promise that represents that
asynchronous task.

Because Juro implements this behaviour, the rejection handling API is designed considering C++'s
exception system: anything thrown is type-erased through a `std::exception_ptr`, and so is the
type of the parameter taken in rejection handling functors.

The most obvious way to attach such a handler is through `.rescue()`:

```C++
#include <string>

using namespace std::string_literals;

juro::make_promise([] (auto &promise) {
    loop.schedule([promise] { promise->reject("Here lies an error"s); }, 1000);
})
// std::exception_ptr is a type-erased pointer
->rescue([] (std::exception_ptr &err) { 
   try {
       // by rethrowing the exception pointer we can disambiguate
       std::rethrow_exception(err); 
       
   // a std::string was thrown, we know how to catch it
   } catch(const std::string &message) { 
       std::cout << "Error: " << message << std::endl;
   
   // some other thing we don't know was thrown
   } catch(...) {
       std::cout << "Unknown error" << std::endl;
   }
});

// After one second, will print "Error: Here lies an error".
```

#### Handling resolution and rejection at once

`.then()` can be used to attach two mutually-exclusive handler at once, each one fit for one
settled state. This is useful to split execution path into two:

```C++
#include <ctime>
#include <cstdlib>

std::srand(std::time(nullptr));

juro::make_promise<int>([] (auto &promise) {
    loop.schedule([promise] {
        auto value = std::rand(); // get a random number
        if(value % 2) {
            promise->resolve(value); // resolves if it's even
        } else {
            promise->reject(value); // rejects if it's odd
        }
    }, 1000);
})
->then( 
    // As the promise cannot be both resolved and rejected, only one of these handlers will be
    // invoked: the first handles resolution and the latter handles rejection
    [] (int value) { 
        std::cout << "Promise was resolved with " << value << std::end; 
    },
    [] (std::exception_ptr &err) {
        try {
            std::rethrow_exception(err);
        } catch(int value) {
            std::cout << "Promise was rejected with" << value << std::endl;
        }
    }
);
```

This is the most generalised form of promise handling; `.then(on_resolve)`, `.rescue(on_reject)` 
and `.finally(on_settle)` are all defined in terms of `.then(on_resolve, on_reject)`.

`.finally()` attaches the same handler for both states, so the provided handler must be able to 
identify and access both a promised value, if any, in case of resolution, or an `std::exception_ptr` 
in case of rejection. Hence, the parameter type of the handler is: 
- for a void promise, `std::optional<std::exception_ptr>` 
- for a non-void promise of type `T`, `std::variant<T, std::exception_ptr>`.

#### Handling after settling

As is with Javascript promises, resolving a promise with a value causes the value to be stored
inside the promise object. If a settle handler is installed by then it is called immediately,
otherwise, it is called when installed. Rejecting a promise when no handler is attached results
in an exception that propagates up to the point of rejection.

```C++
auto p1 = juro::make_promise<std::string>([] (auto &promise) {
    promise->resolve("Resolved"s); // Ok, "Resolved"s gets stored inside *p1
});

p1->then([] (std::string &value) {  
    std::cout << "Resolved with " << value << std::endl;
}); // OK, gets called immediately because the promise is already resolved

auto p2 = juro::make_promise([] (auto &promise) {
    promise->reject(); // Not OK, throws juro::promise_error { "Unhandled promise rejection" } 
});

p2->rescue([] (std::exception_ptr err) {
        // ...
}); // Never gets called because an exception has been thrown sooner
```

There is one special case in which `juro::promise`s are different from JS promises: a rejected 
promise can be safely created with `juro::make_rejected()` and no exception will be thrown.
A handler can still be attached later, and when it does, its rejection functor will be invoked.

```C++
auto promise = std::make_rejected("Rejected here"s);
promise->rescue([] (std::exception_ptr &err) {
   try { std::rethrow_exception(err); }
   catch(std::string &message) { std::cout << message << std::endl; }
}); // OK, gets invoked immediately
```

### Promise chaining

One of the most powerful capabilities of promises is its ability to form chains of tasks that mix
synchronous and asynchronous steps.

Each time a settle handler is attached to a promise (via the *chaining functions* `.then()`, 
`.rescue()` or `.finally()`), a new promise representing that step is returned. This promise 
can be used to later handle a value produced by a previous step.

#### Synchronous chaining

Synchronous chaining occurs when a chaining function handler returns anything but a 
`juro::promise_ptr`. This means that the promised value is immediately available and that the
chained promise can be safely resolved:

```C++
#include <format>

// promise #1
juro::make_promise<int>([] (auto &promise) {
    promise->resolve(10);
})

// promise #2
->then([] (int value) {
    return value * 2;
}) // returns a juro::promise_ptr<int>, deduced from the return type

// promise #3
->then([] (int value) {
    return std::format("Got value: {}", value);
}); // returns a juro::promise_ptr<std::string>
```

As every step here is synchronous, all processing occurs during chain construction:

1. The first `int` promise, `#1`, is created and is immediately resolved with the value `10`;
2. As the first call to `.then()` occurs, a new handler is attached to promise `#1`; a new promise 
   `#2` is returned, representing the second processing step;
3. Because the first promise is already resolved, its settle handler is invoked; promise `#2` gets
   resolved with the return value of the resolve handler, `20`;
4. As the second call to `.then()` occurs, a new handler is attached to promise `#2`; a new promise
   `#3` is returned, representing the third processing step;
5. Because the second promise is already resolved, its settle handler is invoked; promise `#3` gets
   resolved with the return value of the resolve handler, `"Got value: 20"s`;

Whenever a settle handler (attached by *any* chaining function) is called, its return value is 
used to resolve the chained promise; anything is thrown in it, however, is captured into an 
`std::exception_ptr` and used to reject the chained promise.

> Because settle handlers can return a value that resolves a chained promise, they can be used to 
> *recover* from exceptional situations, commuting back to the ordinary execution path if 
> possible.
> 
> ```C++
> juro::make_promise<int>([] (auto &promise) {  
>     get_number_async(promise);
> })
> ->then([] (int value) {
>     if(value % 2 != 0) {
>         throw std::runtime_error { "Number is odd!"s };
>     }
>     return value;
> })
> // If anything is thrown up the promise chain, will hit this fence.
> // After the situation is handled, ordinary execution path is resumed. 
> ->rescue([] (auto) {
>     return 0; 
> })
> ->then([] (int value) {
>     std::cout << "Got value: " << value << std::endl;
> });
> ```

#### Asynchronous chaining

If the handler provided to any chaining function returns a `juro::promise_ptr`, *asynchronous
chaining* takes place: The chained promise remains pending and, instead, the promise returned by
the handler when it is called is *piped* into the chained promise. This effectively means that 
once the new promise is settled, the chained promise will be settled in the same manner and with
the same value.

Asynchronous chaining is most useful for coordinating asynchronous tasks that form a single 
asynchronous pipeline:

```C++
// Returns a promise that gets resolved with `value` doubled after a while
juro::promise_ptr<int> double_async(int value) {
    return juro::make_promise<int>([value] (auto &promise) {
        loop.schedule([promise, value] { promise->resolve(value * 2;) }, 1000);
    });
}

// Returns a promise that gets resolved with `value` squared after a while
juro::promise_ptr<int> square_async(int value) {
    return juro::make_promise<int>([value] (auto &promise) {
        loop.schedule([promise, value] { promise->resolve(value * value;) }, 1000);
    });
}

// Returns a promise that gets resolved with `value` incremented by `amount` after a while
juro::promise_ptr<int> increment_async(int value, int amount) {
    return juro::make_promise<int>([value, amount] (auto &promise) {
        loop.schedule([promise, value] { promise->resolve(value + amount;) }, 1000);
    });
}

// returns promise that resolves after 1s with value `20`
double_async(10)
    // returns a chained promise that awaits on the innermost promise; when is settles, this
    // promise will settle with same state and value
    ->then([] (int result) {
        // returns a promise that resolves after 1s with the value `20` squared, `400`
        // this will be piped into the chained promise
        return square_async(result);
    }) 
    // same as before, returns a chained promise that awaits on the innermost promise 
    ->then([] (int result) {
        // returns a promise that resolves after 1s with the value `400` incremented by `23`,
        // `423`
        // this will be piped into the chained promise
        return increment_async(result, 23);
    });
```

This whole chain returns a `juro::promise_ptr<int>` of a promise that gets resolved after 3s, one
second for each step. As each step gets done, a little bit more of the asynchronous chain is 
unrolled until a new asynchronous operation begins, when chain execution is once again halted
while the process is not concluded.

#### Chained promise type

Juro aims to be a type-safe library, especially in the ordinary execution path. This means a little
extra burden is put on the shoulders of the programmer, that must actually **think** about the
type of each promise involved in a chain.

As stated before, all chaining functions are defined in terms of `.then(on_resolve, on_reject)`. It
is by inspecting these two parameters that the type of the chained promise is determined. When
calling `.then(on_resolve)`, `.rescue(on_reject)` and `.finally(on_settle), the chained promise 
is simply deduced from the single handler's return type:

```C++
juro::make_pending()
    // Because this handler returns a `std::string`, `.then()` returns
    // a `juro::promise_ptr<std::string>`.
    ->then([] { 
        return "This is a promise string"s;            
    })
    // Because this handler returns a `juro::promise_ptr<std::string>`,
    // `.then()` returns a `juro::promise_ptr<std::string>`.
    ->then([] (const std::string &) {
        return juro::make_pending<std::string>();
    })
    // Using a trailing type in lambdas provided to chaining functions can 
    // help ensure the promise is created for the desired type.
    // This returns a `juro::promise_ptr<unsigned int>`.
    ->then([] (const std::string &) -> unsigned int {
        return 1u;
    })
    // Because this handler returns void, `.rescue()` returns a
    // `juro::promise_ptr<void>`.
    ->rescue([] (auto) {
        do_something();
    })
    // Because the handler returns an `std::tuple<bool, double>`,
    // `.finally()` returns a `juro::promise_ptr<std::tuple<bool, double>>`.
    ->finally([] (std::optional<std::exception_ptr> &result) -> 
        std::tuple<bool, double> 
    {
        do_other_thing();
        return { true, -37.2 };
    });
```

> **Notice** that if a handler returns a promise pointer of some type `T`, chaining functions
> *unwrap* this promise pointer and recover the original `T` for type deduction. This is why the 
> second `.then()` call in the previous snippet returns a `juro::promise_ptr<std::string>`
> instead of a `juro::promise_ptr<juro::promise_ptr<std::string>>`.
> 
> Following are cases of compound containers that must be deduced; in every instance, types are
> unwrapped before any other consideration.

When attaching two handlers at once with `.then(on_resolve, on_reject)`, the return types of these
handlers, `T1` and `T2`, are unwrapped and analysed, mapping their combination to a container type
suitable to hold both types. The following table illustrates this mapping:

| T1                | T2                | `.then()` return type                     |
|-------------------|-------------------|-------------------------------------------|
| void              | void              | `juro::promise_ptr<void>`                 |                          
| void              | non-void          | `juro::promise_ptr<std::optional<T2>>`    |
| non-void          | void              | `juro::promise_ptr<std::optional<T1>>`    |
| non-void          | non-void          | `juro::promise_ptr<std::variant<T1, T2>>` |
| `T`, same as `T2` | `T`, same as `T1` | `juro::promise_ptr<T>`                    |

This is easy to remember: If both types are equal, the promise is of that same type; else if any
of them is `void`, the promise is of an optional non-void type, else the promise is of a variant
of both types.

```C++
juro::make_pending()
    ->then(
        [] { }, //returns `void`
        [] (auto) { } // returns `void`
    ) // returns `juro::promise_ptr<void>`
    ->then(
        [] { return 1; }, // returns `int`
        [] (auto) { return 1000; } // returns `int`
    ) // returns `juro::promise_ptr<int>`
    ->then(
        [] (int) { return 1; }, // returns `int`
        [] (auto) { } // returns `void`
    ) // returns `juro::promise_ptr<std::optional<int>>`
    ->then(
        [] (auto) { }, // returns `void`
        [] (auto) { return "A string"s; } // returns `std::string`
    ) // returns `juro::promise_ptr<std::optional<std::string>>`
    ->then(
        [] (auto) { return "Another string"s; }, // returns std::string
        [] (auto) { return 1.0f; } // returns float
    ); // returns `juro::promise_ptr<std::variant<std::string, float>>`
```

### Promise composition

There are currently two functions that compose multiple promises in a single one:

#### `juro::all()`

`juro::all()` takes a variable number of promises and maps their resolved values to an 
`std::tuple`.

```C++
template<class ...T_promises>
auto juro::all(const juro::promise_ptr<T_promises> &...promises);
```

For each promise type `T` in `T_promises...`, `T` is mapped to a suitable storage type `U`: 
if `T` is `void`, `U` is `juro::void_type`; otherwise `U` is `T`. The resolved tuple is of type
`std::tuple<U_promises...>`, where `U_promises` is a parameter pack containing each `U`, in the
same order as their mapped `T`.

```C++
#include <juro/compose/all.hpp>

// returns juro::promise_ptr<std::tuple<int, std::string, juro::void_type>>
juro::all(
    juro::make_pending<int>(), 
    juro::make_pending<std::string>(), 
    juro::make_pending()
)
->then([] (auto &result) {
    auto [ number, text, nil ] = result;
    // number is int, text is std::string, nil is juro::void_type
});
```

Once *all* provided promises are resolved, the composed promise is resolved with the corresponding
tuple type. If *any* promise rejects, however, the composed promise will be rejected with that
same error parameter. Only the first rejection is handled, subsequent errors are silently 
swallowed.

`juro::all()` has a special behaviour when all provided promises are of type `void`. In this case,
it will return simply a `juro::promise_ptr<void>`.

```C++
// because all three promises are `void`, returns `juro::promise_ptr<void>`
juro::all(
    juro::make_pending(), 
    juro::make_pending(), 
    juro::make_pending()
)
->then([] { std::cout << "All resolved!" << std::endl; });
```

#### `juro::race()`

`juro::race()` takes a variable number of promises and returns a composed promise that is resolved 
as soon as any of the provided promises is resolved, with the same value.

```C++
template<class ...T_promises>
auto juro::race(const juro::promise_ptr<T_promises> &...promises);
```

The type of the composed promise is a storage type suitable for storing a resolved value from *any*
of the provided promises. To deduce this container, a unique list of types in `T_promises...` is
extracted and then mapped to suitable storage types, similarly to what happens in `juro::all()`: 
for each unique type `T` in `T_promises...`, we determine a type `U` that is `T` is `T` is 
non-void and `juro::void_type` otherwise. The parameter pack of unique, storage-mapped types is
`U_promises`.

`juro::race()` then proceeds to return an `std::variant<U_promises...>`.

```C++
juro::race(
    juro::make_pending<int>(),
    juro::make_pending<std::string>(),
    juro::make_pending()
)
->then([] (auto &result) {
    // result is a std::variant<int, std::string, juro::void_type>
});
```

```C++
juro::race(
    juro::make_pending<int>(),
    juro::make_pending<bool>(),
    juro::make_pending<bool>(),
    juro::make_pending<std::string>()
)
->then([] (auto &result) {
    // result is a std::variant<int, bool, std::string>
});
```

The first promise to settle will also cause the composed promise to be settled in the same way;
any subsequent settling, whether resolution or rejection, will be silently swallowed.

Unlike `juro::all()`, `juro::race()` does not yet implement all-`void` promises special behaviour.

### Promise lifetime and memory management

Promises are meant to be immovable objects accessed solely through a `juro::promise_ptr`. 
Whenever a promise is created through any of the factory functions, a copy of its shared pointer
is returned. No copies of this handler is stored anywhere by the library, so the promise will live 
on as long as the programmer keeps one of these.

In most scenarios, the handle will be used in two different moments:
- On promise creation, most of the time, the subsequent steps that are to be executed are already
  known. The handle should be used to construct a promise chain using *chaining functions*;
- Any *executor* of the asynchronous task (*i.e.*: whatever program module actually does the heavy
  lifting) should be provided **and store** a copy of the `juro::promise_ptr`, at least until
  settle time. Not only this is the only way of settling the promise in the future, but it also 
  ensures memory is properly handled -- no invalid accesses and no memory leaks must occur.

The following snippet illustrates this typical scenario:

```C++
juro::promise_ptr<int> async_number_generator() {
    // Creates a promise; the only copy of its shared pointer is in the stack
    // and the promise will be deleted unless it gets stored elsewhere
    return juro::make_promise<int>([&] (auto &promise) { 
        // Here, the shared pointer is **copied** into the lambda; because
        // of this, while the event loop keeps a copy of the lambda, the
        // promise will also be alive (and ready for settling)
        loop.schedule([promise] {
            // As `promise` is actually a `juro::promise_ptr<int>`, a.k.a. 
            // `std::shared_ptr<juro::promise<int>>`, the promise object is
            // definitely valid
            promise->resolve(100);
        }); 
        // Presuming the event loop deletes the lambda after it is executed,
        // the promise will be deleted after it is resolved and its resolve 
        // handlers are invoked, unless somewhere else a copy is held in memory
    });
}
```

> Besides any external manipulation, chained promises also get their shared pointers stored inside
> their previous promises. This effectively constructs a reference chain that makes possible to 
> keep only pending segments allocated as they are needed by releasing no longer necessary promise
> pointers.

## Roadmap
- [ ] Comprehensive test suite 
- [ ] Comprehensive documentation
  - [x] Complete API comments
  - [ ] Generate and serve Doxygen documentation
  - [x] Complete usage guide
  - [ ] Build instructions
  - [ ] Examples
  - [ ] Add CONTRIBUTING.md / collaboration guides
- [ ] Fill in some API gaps
  - [ ] `juro::all_settled()`
  - [ ] `juro::any()`
- [ ] Add `operator>>` for `juro::promise_ptr` conveniency chaining