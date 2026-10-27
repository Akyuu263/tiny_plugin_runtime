#pragma once

#include <filesystem>
#include <string_view>

#include <tpr/error.hpp>
#include <tpr/plugin_instance.hpp>

namespace tpr {

class PluginRegistry {
public:
    [[nodiscard]]
    Result<void> load(const std::filesystem::path& path);

    [[nodiscard]]
    Result<PluginInstance> create(std::string_view name) const;

    [[nodiscard]]
    Result<void> unregister(std::string_view name);

    [[nodiscard]]
    bool contains(std::string_view name) const;

private:
    // 注册表状态与内部 LoadedPlugin
};

}
