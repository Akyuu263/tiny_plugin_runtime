add_library(tpr STATIC
    loaded_plugin.cpp
    plugin_instance.cpp
    plugin_registry.cpp
)
add_library(tpr::tpr ALIAS tpr)

target_compile_features(tpr PUBLIC cxx_std_23)
target_include_directories(tpr PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(tpr PRIVATE ${CMAKE_DL_LIBS})
set_target_properties(tpr PROPERTIES POSITION_INDEPENDENT_CODE ON)
