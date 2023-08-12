# Fugax

Fugax is an efficient, modern, ergonomic and portable event loop written in C++17.

## Table of Contents

<!-- TOC -->
* [Fugax](#fugax)
  * [Table of Contents](#table-of-contents)
  * [Features](#features)
  * [Quick demo](#quick-demo)
  * [The event loop](#the-event-loop)
    * [Runloops and the execution counter](#runloops-and-the-execution-counter)
      * [Bare metal](#bare-metal)
      * [RTOS / Desktop](#rtos--desktop)
    * [Modes of operation](#modes-of-operation)
      * [Spinning mode](#spinning-mode)
      * [Ticking mode](#ticking-mode)
  * [Event scheduling](#event-scheduling)
    * [Main schedule function](#main-schedule-function)
      * [Event handler](#event-handler)
      * [Schedule policy](#schedule-policy)
      * [Delay](#delay)
    * [Other schedule overloads and functions](#other-schedule-overloads-and-functions)
      * [Immediate scheduling](#immediate-scheduling)
      * [Delayed scheduling](#delayed-scheduling)
      * [Conditionally recurring scheduling](#conditionally-recurring-scheduling)
      * [Continuous execution scheduling](#continuous-execution-scheduling)
  * [Event listeners and event cancellation](#event-listeners-and-event-cancellation)
    * [Event guards](#event-guards)
  * [Other non-core functionality](#other-non-core-functionality)
    * [Throttling and debouncing](#throttling-and-debouncing)
      * [Throttlers](#throttlers)
      * [Debouncers](#debouncers)
    * [Juro integration](#juro-integration)
      * [Waiting](#waiting)
      * [Asynchronous timeouts](#asynchronous-timeouts)
<!-- TOC -->

## Features

- Light, sleek and fast
- Ergonomic and flexible API
- Completely portable
- Fit for embedded designs and desktop applications alike

## Quick demo

```C++
#include <atomic>
#include <iostream>
#include <fugax.hpp>

static std::atomic<fugax::time_type> milliseconds = 0;

// Interrupt or thread kicking in every 1ms
void my_timer_interrupt() {
    milliseconds++;
}

void print_counter() {
    std::cout << "counter: " << milliseconds << "ms" << std::endl;
}

int main(int argc, const char *argv[]) {
    fugax::event_loop loop;
    bool done;
    
    // Schedules any functor to 100ms into the future
    loop.schedule(1000, [] {
        print_counter();
        done = true;
    });
    
    // Can schedule recurring tasks also
    loop.schedule(499, true, [] {
        print_counter();
    });
    
    print_counter();
    
    // Spins the loop, ensuring events are processes ASAP
    while(!done) {
        loop.process(milliseconds);
    }
    
    return 0;
}
```

```
prints immediately
> counter: 0ms

prints after 499 milliseconds
> counter: 499ms

prints after 998 milliseconds
> counter: 998ms

prints after one second
> counter: 1000ms

then exits with status 0
```

## The event loop

The central piece of Fugax is its event loop. It is a data structure that maintains a schedule of
events, represented as functors, indexed by their due time.

Upon continuous stimulation with a constantly increasing execution time counter, it can 
coordinate execution of tasks, serving as both source and destination of asynchrony to an 
application.

### Runloops and the execution counter

The event loop must be continuously stimulated to process any due events. This is done through the
`.process()` member function, that takes a `fugax::time_type` as parameter:

```C++
// This gets incremented every 1ms by an ISR or thread
static std::atomic<fugax::time_type> milliseconds = 0;

// In the main function
while(true) {
    loop.process(milliseconds);
}
```

Each call to `.process()` is called a **runloop**. It comprehends all necessary housekeeping, such 
as collecting due events, executing them and rescheduling any recurring ones. Its sole parameter
is of an integral type and should be monotonically increased over time: the **execution counter**.

Because the execution counter is external to the event loop, as long as there is a way to keep 
track of time it can be easily integrated to many kinds of systems.

#### Bare metal

When running on a bare metal system, having a 1kHz interruption increment the counter is fairly 
easy. On a multitude of system it is also possible to have a 1kHz hardware counter that can be 
also directly fed to the loop. It doesn't matter.

```C++
// 1kHz interruption
void my_timer_isr() {
    milliseconds++;
}
```

#### RTOS / Desktop

On a RTOS, a worker thread can be solely responsible for incrementing the counter. Likewise,
the main thread can simply keep reading the system clock and calculating the delta between calls
to `.process()`, although this is unnecessary onerous. 

```C++
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

// Worker thread responsible for keeping the execution counter running
std::thread clock_worker { [] {
    
    // Time point when the application started running
    const auto initial = std::chrono::steady_clock::now();
    
    while(true) {
        // Every 1ms or so, wake up and update the timer
        std::this_thread::sleep_for(1ms);
        
        // Because sleeping is not reliable as a source of time counting,
        // we calculate the time difference from the initial counter on 
        // every iteration. This makes up for the interval inaccuracy.
        const auto now = std::chrono::steady_clock::now();
        const auto delta =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - initial);
        milliseconds = delta.count();
    }
} };
```
### Modes of operation

The event loop can be operated on both ticking and spinning modes. These modes will affect major 
aspects of the application, namely: fixed schedule latency and computing time (which translates 
into power consumption). 

#### Spinning mode

So far only the spinning mode has been introduced: threads or interruptions are responsible for 
updating the execution counter and, in the main thread, the event loop *spins* to process 
events as they arrive:

```C++
while(true) {
    loop.process(milliseconds);
}
```

This ensures a minimum fixed schedule latency -- the time between the scheduling of an event for
immediate execution (basically, a timer with zero delay) and its effective execution is simply the 
delay between finishing the current runloop and initiating the next one, when the event's task 
will be executed, plus any time spent running and managing other tasks. 

However, this means that the loop will keep spinning even if there is no work to be done at all.
On many systems this is acceptable, but on those where it is not, ticking mode can be employed.

#### Ticking mode

When running on ticking mode, instead of using a thread or an interruption as a side worker, 
whose purpose is solely to increment our execution counter, we move the whole runloop into the
worker. This means that more events will accumulate over time and be processed in batches, what
can be more forgiving on computing time, but can accumulate latency and deteriorate time precision.

```C++
#include <atomic>
#include <fugax.hpp>

static fugax::event_loop loop;
static std::atomic<fugax::time_type> counter = 0;

// 1kHz interruption, assume the timer keeps running on sleep and
// the interruption wakes the device up
void my_timer_isr() {
    loop.process(++counter);
}

int main(int argc, const char *argv[]) {
    // all configuration and stuff
    
    // nothing else to do with main, put the device to sleep
    my_device_sleep();
    
    return 0;
}
```

In this setup, the device will spend most time sleeping unless there is something heavy to process.
If there are other sources of asynchrony in the system, such as other interruptions that can wake
the device up and schedule tasks in the loop, there is no fixed schedule latency for these tasks, 
but their *maximum* schedule latency is the interruption interval plus housekeeping time.

When scheduling an event from within a runloop -- that is, by any code that is called by the event
loop --, the *minimum* schedule latency is the interruption interval plus housekeeping time.

This same setup can be accomplished with a RTOS by simply putting the main thread to sleep:

```C++
#include <chrono>
#include <thread>
#include <fugax.hpp>

using namespace std::chrono_literals;

int main(int argc, const char *argv[]) {
    fugax::event_loop loop;
    
    const auto initial = std::chrono::steady_clock::now();
    while(true) {
        std::this_thread::sleep_for(1ms);
        
        const auto now = std::chrono::steady_clock::now();
        const auto delta =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - initial);
        loop.process(delta.count());
    }
    
    return 0;
}
```

## Event scheduling

Once a loop is running properly, it will start processing due events as they are scheduled. This is
accomplished mainly through the `.schedule()` function, which comes with various overloads for 
ease of use.

### Main schedule function

The main and most flexible overload of `.schedule()` takes as parameters a delay, a schedule policy
and an event handler, a task functor that will be invoked when the event's due time arrives:

```C++
fugax::event_listener 
    schedule(fugax::time_type delay, fugax::schedule_policy policy, fugax::event_handler functor);
```

#### Event handler

The event handler keeps a reference to  the actual code that will be run when the due time arrives 
and is always a mandatory parameter. 
It can be any callable and, optionally, take a `fugax::event` as parameter, which can be used to 
cancel the event from inside it.

```C++
// This can be an event handler:
auto handler_1 = [] { do_something(); };

// this can also be an event handler:
static void handler_2(fugax::event &event) {
    do_something();
}
```

Other than its signature, the only requirement of the provided functor is that it is 
move-constructible.

#### Schedule policy

The schedule policy is an `enum class` that determines how this event is to be scheduled in the 
event loop. Its possible values are:

- `immediate`: The task will be executed as soon as possible, on the next runloop
- `delayed`: The task will be executed some time into the future
- `recurring_immediate`: The task will be executed periodically; its first execution will
    occur on the next runloop
- `recurring_delayed`: The task will be executed periodically; its first execution will occur
    after the informed delay has passed
- `always`: The task will be executed on **every** runloop until it is cancelled

#### Delay

The delay parameter represents the minimum time that must pass between the scheduling of an
event and its processing.
It is always in system units (so if the event loop is counting milliseconds, so is the delay) and 
is ignored when scheduling with `immediate` or `always` policies.

### Other schedule overloads and functions

There are other overloads of the `.schedule()` function that can be used to more easily schedule
events with specific policies:

#### Immediate scheduling
```C++
// same as schedule(0, fugax::schedule_policy::immediate, functor)
fugax::event_listener schedule(fugax::event_handler functor);
```

#### Delayed scheduling
```C++
// same as schedule(delay, fugax::schedule_policy::delayed, functor)
fugax::event_listener 
    schedule(fugax::time_type delay, fugax::event_handler functor);
```

#### Conditionally recurring scheduling
```C++
// if `recurring`, same as:
// schedule(delay, fugax::schedule_policy::recurring_delayed, functor)
// else, same as:
// schedule(delay, fugax::schedule_policy::delayed, functor)
fugax::event_listener 
    schedule(fugax::time_type delay, bool recurring, fugax::event_handler functor);
```

#### Continuous execution scheduling
```C++
// same as schedule(0, fugax::schedule_policy::always, functor)
fugax::event_listener always(fugax::event_handler functor);
```

## Event listeners and event cancellation

Every scheduling function returns a `fugax::event_listener`, which is merely an alias for a
`std::weak_ptr<fugax::event>`. This listener can be used to cancel the event:

```C++
// schedules the functor to be executed 100ms in the future
auto listener = loop.schedule(100, [] { do_something(); });

if(auto event = listener.lock()) {
    event->cancel();
}
```

Because events are destroyed when they are not needed any more, such as after any non-recurring 
events are executed, locking the listener will fail in these cases and no illegal memory will 
ever be accessed.

Besides cancelling the event from outside, an event's task functor may receive an optional 
`fugax::event &` parameter that directly exposes the event object, so it can be cancelled from 
 within:

```C++
// schedules a recurring functor to be executed every 1000ms
loop.schedule(1000, true, [&] (auto &event) {
    
    // the event must not run any more; cancel it and return
    if(some_critical_async_assumption_has_changed) {
        event.cancel();
        return;
    }
    
    do_something();
});
```

When calling `.cancel()`, an event is marked as invalid and, when its due time arrives, instead of
being processed, it will be simply discarded.

### Event guards

Event guards are RAII containers that manages event listeners, attempting to cancel their source
event when destroyed. They are very useful for consistently ensuring no asynchronous events that
target a dynamic object will ever be fired after the object is destroyed.

They can be created and assigned directly from `fugax::event_listeners` for maximum easiness.

```C++
class my_object {
    std::string name;
    fugax::event_guard guard;
    
public:
    my_object(fugax::event_loop &loop, std::string name) :
        name { name },
        guard { 
            loop.schedule(1000, [this] { greet(); }) 
        }
    {  }
    
    void greet() { 
        std::cout << "Ave, " << name << std::endl; 
    }
};
```

In this snippet, when an instance of `my_object` is created, it will schedule an event to be run
after one second:

```C++
fugax::event_loop loop;
my_object greeter { loop, "Caesar" };

while(true) { 
    loop.process(milliseconds); 
} 
```

```
after one second spinning, will print
> Ave, Caesar
```

If the duration of `greeter` was, instead, dynamic and actually shorter than the event's delay,
the event guard contained in the greeter would automatically cancel the event upon destruction,
ensuring no dangling pointers are accessed:

```C++
fugax::event_loop loop;
auto *greeter = new my_object { loop, "Caesar" };

loop.schedule(500, [greeter] { 
    delete greeter; 
});

while(true) { 
    loop.process(milliseconds); 
}
```

Because the greeter is deleted after 500 milliseconds, the event it scheduled is automatically
cancelled and won't ever be processed.

## Other non-core functionality

### Throttling and debouncing

The event loop provide special template functions that act as throttlers and debouncers of 
wrapped functors.

#### Throttlers

```C++
template<class ...T_args, class T_functor>
auto throttle(time_type delay, T_functor &&functor);
```

This template function gets a functor of type `T_functor` that accepts as parameters the types
`T_args...` and returns a throttled version of the functor. This returned functor has the same
signature as the wrapped one and, even upon frequent invocation, will only invoke the initial 
functor every `delay` units of time; any other calls during this suspension interval will be
silently swallowed.

```C++
fugax::event_loop loop;

auto throttled = loop.throttle(100, [] { 
    std::cout << "running" << std::endl; 
});

throttled(); // prints "running"
throttled(); // nothing happens
throttled(); // nothing happens

loop.process(100);
throttled(); // prints "running"
throttled(); // nothing happens
```

#### Debouncers

```C++
template<class ...T_args, class T_functor>
auto debounce(time_type delay, T_functor &&functor);
```

Similarly to the `.throttle()` function, the `.debounce()` template function receives a delay and a
functor as parameters and returns a wrapping functor with the same signature. When called, instead
of immediately invoking its wrapped functor, the debounced version scheduled an event with `delay`
units of time.

If the debounced functor is not invoked until this event is due, the wrapped functor gets called.
Otherwise, the event is rescheduled for execution after `delay` units of time from the invocation 
occasion. This means that the initial functor will only get called after `delay` has passed without
the debounced version being invoked.

The debounced value gets stores inside the returned functor object and is updated each time it is
invoked, overriding the last value. Because of this, when the initial functor is finally called, 
it will receive the **last** debounced value every time.

```C++
fugax::event_loop loop;

auto debounced = loop.debounce<int>(100, [] (int value) {
    std::cout << "got " << value << std::endl; 
});

debounced(1); // event is scheduled
debounced(2); // event is rescheduled
debounced(3); // event is rescheduled

loop.process(100); // will print "got 3"

debounced(4); // event is scheduled
debounced(5); // event is rescheduled
```

### Juro integration

The event loop can also be used to construct time-dependent promises that can be useful 
primitives in complex asynchronous operations.

#### Waiting
```C++
juro::promise_ptr<fugax::timeout> wait(time_type delay);
````
    
Returns a promise that resolves with an instance of the tag-type `fugax::timeout` after 
`delay` units of time and never rejects.

#### Asynchronous timeouts
```C++
template<class T_value>
auto timeout(time_type delay, const juro::promise_ptr<T_value> &promise);
```

Returns a race promise between the supplied promise and a `.wait()` invocation. When any of
the promises resolves, the race promise will be resolved with that same value; if the
supplied promise rejects, the race promise will be rejected with that same error.

In the attached resolution handler, a timeout situation can be identified by STL utilities.

```C++
fugax::event_loop loop;

loop.timeout(100, juro::make_promise<std::string>([&] (auto &promise) {
    loop.schedule(50, [promise] { 
        promise->resolve("resolved"); 
    });
}))
->then([] (auto &result) {
    if(std::holds_alternative<fugax::timeout>(result)) {
        // timeout occurred
    } else {
        auto &string = std::get<std::string>(result); // "resolved"
    }
});
```

In the previous snippet, as the promise resolution happens before the timeout (50ms vs 100ms),
the result provided to the resolution handler contains a `std::string`. If the resolution was
scheduled further than the timeout, the result would contain a `fugax::timeout`.

A timeout race can also be directly started through the other `.timeout()` overload:

```C++
template<class T_value, class T_launcher>
auto timeout(time_type delay, const T_launcher &launcher);
```

Instead of receiving a promise as parameter, it receives a promise *launcher* and forwards it to
`juro::make_promise`, so a promise can be created inline.

```C++
loop.timeout<std::string>(100, [&] (auto &promise) {
    loop.schedule(50, [promise] { 
        promise->resolve("resolved"); 
    });
});
```