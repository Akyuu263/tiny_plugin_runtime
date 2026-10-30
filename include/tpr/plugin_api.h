#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#include <stdint.h>

#define ABI_V1 1

#define STATUS_OK                0
#define STATUS_INVALID_ARGUMENT  1
#define STATUS_CREATE_FAILED     2
#define STATUS_EXECUTION_FAILED  3

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PluginApi {
    uint32_t abi_version;
    uint32_t struct_size;

    char const* name;

    int32_t (*create)(void** out_instance);
    void (*destroy)(void* instance);

    int32_t (*transform)(
        void* instance,
        int32_t input,
        int32_t* out_result
    );

} PluginApi;

PluginApi const* get_api_v1(void);

typedef PluginApi const* (* PluginApiFn)(void);

#ifdef __cplusplus
}
#endif

#endif
