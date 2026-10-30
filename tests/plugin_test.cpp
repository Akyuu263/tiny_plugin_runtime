#include <cstdint>
#include <cstdio>
#include <dlfcn.h>
#include <tpr/plugin_api.h>

namespace {

int fail(char const* what) {
    std::fprintf(stderr, "FAIL:%s\n", what);
    return 1;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        return fail("usage: plugin_test <plugin.so>");
    }

    void* handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
        std::fprintf(stderr, "FAIL: dlopen: %s\n", dlerror());
        return 1;
    }

    dlerror();
    auto const get_api = reinterpret_cast<PluginApiFn>(dlsym(handle, "get_api_v1"));
    auto const symbol_error = dlerror();
    if (symbol_error != nullptr) {
        std::fprintf(stderr, "FAIL: dlsym: %s\n", symbol_error);
        dlclose(handle);
        return 1;
    }
    if (get_api == nullptr) {
        dlclose(handle);
        return fail("null get_api_v1 entry point");
    }

    PluginApi const* api = get_api();
    if (api == nullptr) {
        dlclose(handle);
        return fail("get_api_v1");
    }

    if (api->abi_version != ABI_V1) {
        dlclose(handle);
        return fail("abi_version mismatch");
    }

    if (api->struct_size != sizeof(PluginApi)) {
        dlclose(handle);
        return fail("struct_size mismatch");
    }

    if (api->create == nullptr || api->destroy == nullptr || api->transform == nullptr) {
        dlclose(handle);
        return fail("null function pointer in PluginApi");
    }

    if (api->name == nullptr || api->name[0] == '\0') {
        dlclose(handle);
        return fail("empty plugin name");
    }
    std::printf("plugin name: %s\n", api->name);

    void* instance = nullptr;
    if (api->create(&instance) != STATUS_OK) {
        dlclose(handle);
        return fail("create failed");
    }
    if (instance == nullptr) {
        dlclose(handle);
        return fail("create succeeded with a null instance");
    }

    std::int32_t res = 0;
    auto const status = api->transform(instance, 21, &res);
    if (status != STATUS_OK || res != 43) {
        std::fprintf(stderr, "FAIL: transform: status=%d, result=%d, expected=43\n",
                     static_cast<int>(status), static_cast<int>(res));
        api->destroy(instance);
        dlclose(handle);
        return 1;
    }

    api->destroy(instance);
    dlclose(handle);

    std::printf("OK\n");
    return 0;
}
