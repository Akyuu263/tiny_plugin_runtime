#include "loaded_plugin.hpp"
#include "tpr/error.hpp"
#include "tpr/plugin_api.h"
#include <dlfcn.h>
#include <string>

namespace tpr {
LoadedPlugin::LoadedPlugin(LibHandle handle, PluginApi const* api) {

}

Result<LoadedPlugin> LoadedPlugin::load(std::filesystem::path const& path) {
    void* raw_open = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (raw_open == nullptr) {
        return std::unexpected(PluginError(
            ErrorCode::LibraryOpenFailed,
            "failed to open plugin library: " + std::string(dlerror()),
            0
        ));
    }

    dlerror();

    LibHandle handle_{raw_open, &dlclose};

    void* raw_sym = dlsym(handle_.get(), "get_api_v1");
    if (char const* error = dlerror()) {
        return std::unexpected(PluginError(
            ErrorCode::EntryPointMissing,
            "failed to resolve plugin entry point get_api_v1: " + std::string(error),
            0
        ));
    }

    if (raw_sym == nullptr) {
        return std::unexpected(PluginError(
            ErrorCode::InvalidPluginApi,
            "plugin entry point get_api_v1 is null",
            0
        ));
    }

    auto api = reinterpret_cast<GetApiFn>(raw_sym)();
    if (api == nullptr) {
        return std::unexpected(PluginError(
            ErrorCode::InvalidPluginApi,
            "plugin entry point get_api_v1 returned a null API table",
            0
        ));
    }

    if (api->abi_version != ABI_V1) {
        return std::unexpected(PluginError(
            ErrorCode::IncompatibleAbi,
            "plugin ABI version does not match ABI_V1",
            0
        ));
    }

    if (api->struct_size != sizeof(PluginApi)) {
        return std::unexpected(PluginError(
            ErrorCode::IncompatibleAbi,
            "plugin API struct size does not match sizeof(PluginApi)",
            0
        ));
    }

    if (api->name == nullptr || api->name[0] == '\0') {
        return std::unexpected(PluginError(
            ErrorCode::InvalidPluginApi,
            "plugin name is null or empty",
            0
        ));
    }

    if (api->create == nullptr) {
        return std::unexpected(PluginError(
            ErrorCode::InvalidPluginApi,
            "plugin create function is null",
            0
        ));
    }

    if (api->destroy == nullptr) {
        return std::unexpected(PluginError(
            ErrorCode::InvalidPluginApi,
            "plugin destroy function is null",
            0
        ));
    }

    if (api->transform == nullptr) {
        return std::unexpected(PluginError(
            ErrorCode::InvalidPluginApi,
            "plugin transform function is null",
            0
        ));
    }

}
}
