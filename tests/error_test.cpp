#include <expected>
#include <string>

int main() {
    std::expected<int, std::string> success = 42;

    if (!success || *success != 42)
        return 1;

    std::expected<int, std::string> failure =
        std::unexpected(std::string("test error"));

    if (failure || failure.error() != "test error")
        return 2;

    return 0;
}
