#pragma once

#include <cstdint>
#include <expected>
#include <string>

namespace tpr {

enum class ErrorCode {
    LibraryOpenFailed,
    EntryPointMissing,
    IncompatibleAbi,
    InvalidPluginApi,
    DuplicatePlugin,
    PluginNotFound,
    InstanceCreationFailed,
    PluginExecutionFailed,
    InvalidInstance
};

struct PluginError {
    ErrorCode code;
    std::string message;
    std::int32_t native_status = 0;
};

template<typename T>
using Result = std::expected<T, PluginError>;

}
