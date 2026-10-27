cmake_minimum_required(VERSION 3.20)
project(tiny_plugin_runtime VERSION 0.1.0 LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "tiny_plugin_runtime v0.1 currently supports Linux only")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles([[
    #include <expected>
    int main() {
        std::expected<int, int> value = 42;
        return value.value() == 42 ? 0 : 1;
    }
]] TPR_HAS_STD_EXPECTED)
if(NOT TPR_HAS_STD_EXPECTED)
    message(FATAL_ERROR "A C++23 compiler and standard library with std::expected are required")
endif()

option(TPR_BUILD_EXAMPLES "Build the example host" ON)
option(TPR_BUILD_PLUGINS "Build the example plugins" ON)

add_subdirectory(src)

if(TPR_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(TPR_BUILD_PLUGINS)
    add_subdirectory(plugins)
endif()

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
