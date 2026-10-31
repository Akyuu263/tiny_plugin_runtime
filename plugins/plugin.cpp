#include <limits>
#include <tpr/plugin_api.h>
#include <new>

namespace {
struct Instance {
    int32_t multiplier = 2;
    int32_t bias = 1;
};

int32_t create(void** out_instance) noexcept {
    if (!out_instance) {
        return STATUS_INVALID_ARGUMENT;
    }

    *out_instance = new (std::nothrow) Instance{};
    return *out_instance ? STATUS_OK : STATUS_CREATE_FAILED;
}

void destroy(void* instance) noexcept {
    delete static_cast<Instance*>(instance);
}

int32_t transform(
        void* instance,
        int32_t input,
        int32_t* out_result
    ) noexcept {
    if (!instance || !out_result) {
        return STATUS_INVALID_ARGUMENT;
    }

    auto const* state = static_cast<Instance*>(instance);
    int64_t const res = static_cast<int64_t>(input) * state->multiplier + state->bias;
    if (res < std::numeric_limits<int32_t>::min() || res > std::numeric_limits<int32_t>::max()) {
        return STATUS_INVALID_ARGUMENT;
    }

    *out_result = res;
    return STATUS_OK;
}

static const PluginApi api = {
    .abi_version = ABI_V1,
    .struct_size = sizeof(PluginApi),
    .name = "test",
    .create = create,
    .destroy = destroy,
    .transform = transform
};

} // namespace


extern "C" PluginApi const* get_api_v1(void) {
    return &api;
}

