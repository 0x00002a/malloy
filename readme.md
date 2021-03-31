# Overview
Malloy is a small, embeddable HTTP & WebSocket server built on top of [boost.beast](https://www.boost.org/doc/libs/1_75_0/libs/beast/doc/html/index.html).

# Features
Currently implemented:
- HTTP Server
- WebSocket Servers
- Request router:
  - Simple handlers
  - Sub-routers
  - Redirections
  - File serving locations

Following soon™:
- Support for TLS connections

# Licensing
This library is MIT licensed. Dependencies ship with their own licensing models.

# Requirements
Building (and using) this library requires:
- A C++20 capable compiler
- CMake 3.17 or newer
  
## Dependencies
Malloy comes with a small number of dependencies:
- [Boost](https://www.boost.org/) 1.73.0 or newer
- [spdlog](https://github.com/gabime/spdlog) 1.8.3 or newer

# Motivation
This started off with the intention of creating a more complex, real-world example of how to use `boost.beast`.

# Documentation
Available documentation sources:
- API documentation (doxygen)

## Doxygen
The Doxygen API documentation can be generated with little hassle:
```shell
doxygen ./Doxyfile
```
The generated output(s) may be found under `/docs/doxygen`. Simply open `/docs/doxygen/html/index.html` in the web browser of your choosing.

# Integration
Malloy is designed to be an embeddable component for other C++ applications.
The easiest way to integrate Malloy is via CMake's `FetchContent()` infrastructure:
```cmake
FetchContent_Declare(
    malloy
    GIT_REPOSITORY https://github.com/tectu/malloy
    GIT_TAG        main
)
FetchContent_MakeAvailable(malloy)
```
If you like to modify set some of Malloy's CMake variables, the `FetchContent_MakeAvailable()` call can be replaced accordingly:
```cmake
FetchContent_Declare(
    malloy
    GIT_REPOSITORY https://github.com/tectu/malloy
    GIT_TAG        main
)
FetchContent_GetProperties(malloy)
if(NOT malloy_POPULATED)
    FetchContent_Populate(malloy)
    set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
    set(BUILD_TESTS OFF CACHE INTERNAL "")
    add_subdirectory(${malloy_SOURCE_DIR} ${malloy_BINARY_DIR})
endif()
```
You may replace `GIT_TAG` with a commit has or a release tag such as `v1.0.0`.

After fetching the content, it's only a matter of linking the malloy library target to the client application:
```cmake
target_link_libraries(
    my_application
    PRIVATE
        malloy-objs
)
```
Where `my_application` is your application (or library) target that should consume malloy.
