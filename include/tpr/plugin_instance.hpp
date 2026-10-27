#pragma once

#include <cstdint>

#include <tpr/error.hpp>

namespace tpr {

class PluginInstance {
public:
    PluginInstance() = delete;

    PluginInstance(const PluginInstance&) = delete;
    PluginInstance& operator=(const PluginInstance&) = delete;

    PluginInstance(PluginInstance&&) noexcept;
    PluginInstance& operator=(PluginInstance&&) noexcept;

    ~PluginInstance() noexcept;

    [[nodiscard]]
    Result<std::int32_t> transform(std::int32_t input);

private:
    // 内部工厂构造实例
};

}
