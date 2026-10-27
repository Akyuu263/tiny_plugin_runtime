# MODULE libraries are loaded by the runtime through dlopen().
add_library(plugin MODULE plugin.cpp)
add_library(failing_plugin MODULE failing_plugin.cpp)

foreach(target IN ITEMS plugin failing_plugin)
    target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/include")
endforeach()
