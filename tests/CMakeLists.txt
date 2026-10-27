add_executable(error_test error_test.cpp)
target_link_libraries(error_test PRIVATE tpr::tpr)

add_test(NAME tpr.error COMMAND error_test)
