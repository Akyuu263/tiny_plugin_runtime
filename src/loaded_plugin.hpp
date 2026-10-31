#pragma once

#include "tpr/error.hpp"
#include "tpr/plugin_api.h"
#include <dlfcn.h>
#include <filesystem>

namespace tpr {
class LoadedPlugin {
public:
    static Result<LoadedPlugin> load(std::filesystem::path const& path);

    LoadedPlugin(LoadedPlugin const&) = delete;
    LoadedPlugin& operator=(LoadedPlugin const&) = delete;

private:
    using LibHandle = std::unique_ptr<void, decltype(&dlclose)>;

    using GetApiFn = PluginApi const* (*)(void);

    LibHandle handle{nullptr, &dlclose};

    LoadedPlugin(LibHandle handle, PluginApi const* api);
};
}
