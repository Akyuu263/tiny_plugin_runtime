add_executable(error_test error_test.cpp)
target_link_libraries(error_test PRIVATE tpr::tpr)

add_test(NAME tpr.error COMMAND error_test)


add_executable(plugin_test plugin_test.cpp)
target_include_directories(plugin_test PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(plugin_test PRIVATE ${CMAKE_DL_LIBS})

if(TARGET plugin)
    add_dependencies(plugin_test plugin)
    add_test(NAME tpr.plugin COMMAND plugin_test $<TARGET_FILE:plugin>)
endif()
