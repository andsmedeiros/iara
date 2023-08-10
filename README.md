# Iara

![cmake build status](https://github.com/andsmedeiros/iara/actions/workflows/cmake.yml/badge.svg)

Iara is a modular framework for composing asynchronous systems in C++17. It is distributed in 
form of several core libraries and some top-level glue code to stitch all together.

Iara is portable, unopinionated, light, memory safe and flexible. It can be made to run in
whatever gets a decent enough compiler support.

## Disclaimer from the creator

Iara is a very WIP. It is born out of need for formalisation of some libraries I developed for
internal use and that, even though had been successfully integrated into production environments
for years, lacked proper testing, documentation and refactoring needed.

As such, most of the available libraries are fully functional, very well tested and many of them
are extremely well documented, both with source annotations and usage guides. However, there is
still some work to be done in the libraries, as well as there are other custom libraries I would
like to create or integrate to this project.

Furthermore, I still envision a bunch of functionality to be implemented by a top-level 
application container under the `iara` namespace; this is still to be developed.

## Project libraries

### Fugax

[Fugax](https://github.com/andsmedeiros/iara/tree/main/fugax) implements a sleek event loop
that can be employed on various platforms. It provides time-based scheduling functionality and
with a very enjoyable API and is used to orchestrate asynchronous task execution from a single, 
central control point.

Fugax has an extensive test suite and excellent documentation. It has also been successfully
used in commercial projects out in the wild.

### FUSS

[FUSS](https://github.com/andsmedeiros/iara/tree/main/fuss) is an in-process pub/sub system. It
allows for objects to subscribe to specific messages broadcast by other objects, even if all 
these objects are completely unrelated and know nothing more about each other, only by adhering 
to a type-based contract.

FUSS is well tested and its code is well annotated; its usage guide is still lacking, however.
Nonetheless, it has also been used successfully in commercial projects.

### Juro

[Juro](https://github.com/andsmedeiros/iara/tree/main/juro) implements Javascript promises with
a very strong typing assurance. It is light, efficient, cleverly implemented, and closely follows 
the original Javascript API.

Juro's documentation is outstanding and its test suite is very comprehensive. It is the latest
addition to my asynchrony-related libraries and the utter motivation for Iara to exist, but is
already being employed on a commercial project that is about to be released.

### Plumbing

[Plumbing](https://github.com/andsmedeiros/iara/tree/main/plumbing) is a legacy library with
subpar functionality. It implements object pull streams, so that producers can yield their
products to a pipeline of transformations, which will eventually lead to a consumer.

Although the idea behind plumbing is excellent, its implementation is lacking and its API is not
well thought. Also, there are no tests and no documentation, along a lot of dead and 
misfunctioning code. Even though I have used it in commercial projects, I do not feel its
implementation complies to my standards nowadays, so I strongly discourage using it.

**THEN WHY IS THIS S\*\*\* HERE???**

It is here as a placeholder for the upcoming revamp this idea will get. The concept is good,
back then I was not.

### Utils

There are some [generic utils](https://github.com/andsmedeiros/iara/tree/main/utils) that
are used through the framework, along with some legacy, unused, sometimes misfunctioning -- but 
still interesting, though -- code. This will eventually deserve a cleanup and some well care, 
however, it currently sits at the bottom of the list.

## Building

The project builds by default with CMake and, besides Catch2 as the test library, there are no
dependencies, so all the needed stuff is likely installed already.

```
~$ git clone https://github.com/andsmedeiros/iara
~$ cd iara

~/iara$ mkdir build
~/iara$ cd build

~/iara/build$ cmake --preset=default ..
~/iara/build$ cmake --build .
```

This will create:
```
iara/
    dist/
        bin/
            iara-test
        lib/
            libfugax.[so/a]
            libjuro.[so/a]
            libiara.[so/a]
```

`iara-test` is the test suite; all `lib*` files are the library files that, in addition to 
include headers, are necessary to use each library. `libiara` is just the other libraries
amalgamated, for now, and can be used along each library's include directory to provide all
libraries at once.

### Build configuration

Some build-time configuration is available. They can be customised by either providing a preset
file 

```
~/iara/build$ cmake --preset=MY_PRESET_NAME ..
```

or passing environment variables during configuration phase

```
~/iara/build$ OPTION_1=value1 OPTION_2=value2 cmake ..
```

At the moment, only Fugax employs this mechanism. These are the available build options:

- `FUGAX_TIME_INCLUDE` if defined, will be directly appended to a `#include` directive and
    can be used to determine a header file that contains the definitions for Fugax's time
    type.
- `FUGAX_MUTEX_INCLUDE` likewise, if defined, is expected to alias the path to a header file
    that contains Fugax's mutex type.
- `FUGAX_TIME_TYPE` **[required]** must alias an integral, unsigned type to hold Fugax's 
    internal counter. The choice of this type can interfere with the maximum delay an event 
    can have.
- `FUGAX_MUTEX_TYPE` the `BasicLockable` type that will be used to declare Fugax's event loop
    internal mutex. Even though name mutex, this can be any structure that can ensure a 
    critical section does not get preempted, such as by disabling and re-enabling exceptions 
    in embedded systems.

### Building without CMake

Building without CMake is fairly easy:

- Manually edit any files under `config/` tree with your desired configuration -- the files
    should be easy to understand and edit anyway --, then rename them from `*.hpp.in` to 
    `*.hpp`.

To target `libfugax`, compile everything under `fugax/src`, while including `fugax/include`,
`config/include`, `juro/include` and `utils/include`.

To target `libjuro`, compile everything under `juro/src`, while including `juro/include` and 
`utils/include`.

FUSS is header-only and Plumbing is not a currently supported target.

Each library has a test directory under `test/src` and `test/include`. Their test suites can be
build using Catch2 and linking against their respective libraries, but this is unsupported out
of CMake.