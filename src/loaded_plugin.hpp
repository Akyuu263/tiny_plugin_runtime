#pragma once

#include "tpr/error.hpp"
#include "tpr/plugin_api.h"
#include <dlfcn.h>
#include <filesystem>
#include <memory>

namespace tpr {
class LoadedPlugin {
public:
    static Result<LoadedPlugin> load(std::filesystem::path const& path);

    LoadedPlugin(LoadedPlugin const&) = delete;
    LoadedPlugin& operator=(LoadedPlugin const&) = delete;

    LoadedPlugin(LoadedPlugin&&) = default;
    LoadedPlugin& operator=(LoadedPlugin&&) = default;

private:
    using LibHandle = std::unique_ptr<void, decltype(&dlclose)>;

    using GetApiFn = PluginApi const* (*)(void);

    LibHandle m_handle;
    PluginApi const* m_api;
    std::string m_name{m_api->name};

    LoadedPlugin(LibHandle&& handle, PluginApi const* api);
};
}
