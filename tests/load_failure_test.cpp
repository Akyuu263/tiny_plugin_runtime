#include "loaded_plugin.hpp"
#include <iostream>

int fail(char const* what) {
    std::fprintf(stderr, "FAIL:%s\n", what);
    return 1;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        return fail("usage: load_failure_test<failing_plugin.so>");
    }
}
