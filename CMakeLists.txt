cmake_minimum_required(VERSION 3.17)

# Project
project(
    malloy
    LANGUAGES CXX
    VERSION 0.0.0
)

# Options
option(MALLOY_BUILD_SHARED   "Whether to build as a shared library" OFF)
option(MALLOY_BUILD_EXAMPLES "Whether to build examples"       ON)
option(MALLOY_BUILD_TESTS    "Whether to build tests"          ON)
option(MALLOY_FEATURE_CLIENT "Whether to build the client"     ON)
option(MALLOY_FEATURE_SERVER "Whether to build the server"     ON)
option(MALLOY_FEATURE_HTML   "Whether to enable HTML features" ON)
option(MALLOY_FEATURE_TLS    "Whether to enable TLS features"  OFF)
option(MALLOY_DEPENDENCY_SPDLOG_DOWNLOAD "Whether to automatically fetch spdlog" ON)
option(MALLOY_DEPENDENCY_FMT_DOWNLOAD "Whether to automatically fetch fmt" ON)

set(MALLOY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin) # Needed for later

if (MSVC AND MALLOY_BUILD_SHARED)
    set(MALLOY_EXPORT_SYMBOLS ON)
else()
    set(MALLOY_EXPORT_SYMBOLS OFF)
endif()

if (MALLOY_EXPORT_SYMBOLS)
    set(MALLOY_TMP_WINEXPORT_ALL ${CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS})
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON) # See: https://cmake.org/cmake/help/latest/prop_tgt/WINDOWS_EXPORT_ALL_SYMBOLS.html
endif()

# Set library type to build
if (MALLOY_BUILD_SHARED) 
    set(MALLOY_LIBRARY_TYPE SHARED)
else()
    set(MALLOY_LIBRARY_TYPE STATIC)
endif()

# Set dependencies accordingly
if (MALLOY_FEATURE_TLS)
    set(MALLOY_DEPENDENCY_OPENSSL ON)
endif()

# Add the main library
add_subdirectory(lib)

# Add examples (if supposed to)
if (MALLOY_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Add tests (if supposed to)
if (MALLOY_BUILD_TESTS)
    add_subdirectory(test)
endif()

# Print options
message("")
message("---------------------")
message("Malloy configuration:")
message("  Build:")
message("    Examples     : " ${MALLOY_BUILD_EXAMPLES})
message("    Tests        : " ${MALLOY_BUILD_TESTS})
message("    Library Type : " ${MALLOY_LIBRARY_TYPE})
message("  Features:")
message("    Client       : " ${MALLOY_FEATURE_CLIENT})
message("    Server       : " ${MALLOY_FEATURE_SERVER})
message("    HTML         : " ${MALLOY_FEATURE_HTML})
message("    TLS          : " ${MALLOY_FEATURE_TLS})
message("---------------------")
message("")

if (MALLOY_EXPORT_SYMBOLS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ${MALLOY_TMP_WINEXPORT_ALL}) # Reset global var
endif()
