# mini-plugin-runtime — Architecture Design v0.1

> **Status:** Design baseline frozen for implementation  
> **Language:** C++23 (public runtime API), C-compatible ABI V1 (plugin boundary)  
> **Platform:** Linux / ELF  
> **Scope:** Trusted plugins, single-threaded management and invocation, no hot replacement

## 1. Project goal

Build a small C++23 runtime that loads ELF shared libraries dynamically, validates a fixed plugin ABI, registers plugins by logical name, creates plugin instances, and releases resources safely without exposing `dlopen`, `dlsym`, or `dlclose` to application developers.

**Explicitly out of scope for v0.1:** Windows/macOS support; untrusted-code isolation; concurrent use of Registry or Instance; hot reload; coexistence guarantees for distinct code generations; cross-version ABI compatibility; recursive plugin dependency management; automated directory discovery; crash recovery. Keep the plugin file immutable while it is in use. Do not use v0.1 to register replacement implementations while older versions' instances remain live.

## 2. Vocabulary and identity

- **Registration:** a name → LoadedPlugin entry in a Registry. Unregister removes this *logical* relationship.
- **LoadedPlugin:** internal object owning a successful `dlopen` reference and a validated ABI function table.
- **PluginInstance:** public move-only owner of a successful plugin-created opaque instance, plus a shared lifetime dependency on its LoadedPlugin.
- **ABI version:** version of the host/plugin binary contract (strict V1 for this release), *not* a plugin implementation version or generation.
- **Generation:** conceptual identity of one registration, not a distinct class or a guarantee of independent ELF loading. No generation APIs are required in v0.1.

## 3. Component responsibilities

| Component | Public? | Responsibilities | Explicitly not responsible for |
|---|---|---|---|
| `PluginRegistry` | Yes | `load`, `create`, `unregister`, `contains`; owned logical registration map | Owning all previously created instances; plugin business logic |
| `LoadedPlugin` | No | `dlopen` reference, entry lookup, ABI V1 validation, function table lifetime; internal factory coordination | Global singleton management; full isolation of plugin globals/dependencies |
| `PluginInstance` | Yes | Unique opaque instance ownership; transform calls; one destruction protocol; move operations; retain LoadedPlugin lifetime | Registry access; creating other instances |

Do not split `SharedLibrary`, `PluginManager`, or `PluginFactory` into public abstractions without concrete new needs. `LoadedPlugin` may internally encapsulate a small RAII shared-library handle.

## 4. Ownership and lifetime

```text
Registry -------- shared_ptr --------┐
                                    LoadedPlugin ---- unique dlopen reference
PluginInstance -- shared_ptr --------┘
       |
       └------- unique opaque instance handle
```

- Registry destruction or unregister only removes *its* shared reference.
- PluginInstance holds a separate shared reference, keeping `LoadedPlugin` alive until its `destroy(instance)` call completes.
- PluginInstance has no public default constructor; is noncopyable and movable.
- A moved-from PluginInstance is **Empty**, safe to destroy and move-assign; `transform` on Empty reports `InvalidInstance`.
- A move assignment must release the destination's previous instance correctly and then take the source's resource, even if destination already owns a different plugin. Self-move is safe.
- Never call a plugin function or destroy an opaque instance after the last valid loader reference has been released.
- `dlclose` releases one reference; it does not promise immediate unmapping. A distinct `shared_ptr` control block does not imply a distinct loaded ELF object.

## 5. Required invariants and postconditions

| ID | Contract |
|---|---|
| I1 | During any plugin function call, including `destroy`, an owning LoadedPlugin reference remains alive. |
| I2 | Every successfully created opaque instance has exactly one framework destruction owner, and its `destroy` is invoked once during orderly cleanup. |
| I3 | Registry contains only plugins that passed the agreed ABI validation steps. |
| I4 | Removing a registration does not invalidate existing instances (within supported v0.1 usage). |
| I5 | Failed `load` leaves Registry's logical registration map unchanged. |
| I6 | Failed `create` does not expose a partly initialized instance or leak plugin-owned partial resources (under the trusted-plugin ABI contract). |
| I7 | Moved-from instances do not own their previous opaque resource and remain safely destructible. |
| I8 | `PluginRegistry` owns registrations, not the lifetimes of previously returned instances. |

**Limitations:** I2 assumes normal stack/process cleanup and a plugin obeying ABI rules; it is not a guarantee against process abort or malicious code. I5 does not roll back side effects of ELF constructors run by `dlopen`.

## 6. Public C++23 API — declarations only, not an implementation

```cpp
namespace mpr {
    enum class ErrorCode {
        LibraryOpenFailed, EntryPointMissing, IncompatibleAbi,
        InvalidPluginApi, DuplicatePlugin, PluginNotFound,
        InstanceCreationFailed, PluginExecutionFailed, InvalidInstance
    };

    struct PluginError {
        ErrorCode code;
        std::string message;
        std::int32_t native_status = 0;
    };

    template<class T>
    using Result = std::expected<T, PluginError>;

    class PluginInstance {
    public:
        PluginInstance() = delete;
        PluginInstance(const PluginInstance&) = delete;
        PluginInstance& operator=(const PluginInstance&) = delete;
        PluginInstance(PluginInstance&&) noexcept;
        PluginInstance& operator=(PluginInstance&&) noexcept;
        ~PluginInstance() noexcept;
        [[nodiscard]] Result<std::int32_t> transform(std::int32_t input);
    private:
        // Private factory constructor and ownership state; details intentionally omitted.
    };

    class PluginRegistry {
    public:
        [[nodiscard]] Result<void> load(const std::filesystem::path& path);
        [[nodiscard]] Result<PluginInstance> create(std::string_view name) const;
        [[nodiscard]] Result<void> unregister(std::string_view name);
        [[nodiscard]] bool contains(std::string_view name) const;
    private:
        // Registration map and other internal details intentionally omitted.
    };
}
```

**Headers needed by this sketch:** `<cstdint>`, `<expected>`, `<filesystem>`, `<string>`, `<string_view>`; function bodies and ownership members are intentionally absent. Only routine/anticipated errors are modeled with `Result`; allocation failures such as `std::bad_alloc` can still propagate as exceptions. APIs do not claim universal `noexcept`.

### Public API state contracts

| Method | Success | Anticipated failure | State effect |
|---|---|---|---|
| `load(path)` | `Result<void>` success | Open/lookup/ABI/duplicate-name error | On failure, registration map unchanged |
| `create(name)` | New Active, move-only instance | Unknown plugin or plugin create failure | Registry unchanged |
| `unregister(name)` | Registration removed | `PluginNotFound` | Existing instance owners unaffected |
| `contains(name)` | Whether current Registry has name | No normal error return | Read-only |
| `transform(input)` | Computed `int32_t` | Plugin status or `InvalidInstance` | On failure, no usable output is returned |

For duplicates, v0.1 rejects a second `load` with the same logical name; it does not replace the current entry. The name returned by the ABI is copied to a Runtime-owned `std::string` before relying on it as a registry key.

## 7. ABI V1 — contract and signatures only

Plugin headers must be consumable from C and C++. Plugin code must export one symbol with **C language linkage**:

```c
#define MPR_ABI_V1 1u
#define MPR_STATUS_OK 0
#define MPR_STATUS_INVALID_ARGUMENT 1
#define MPR_STATUS_CREATE_FAILED 2
#define MPR_STATUS_EXECUTION_FAILED 3

typedef struct MprPluginApiV1 {
    uint32_t abi_version;
    uint32_t struct_size;
    const char* name;
    int32_t (*create)(void** out_instance);
    void (*destroy)(void* instance);
    int32_t (*transform)(void* instance, int32_t input, int32_t* out_result);
} MprPluginApiV1;

/* Export with C linkage in a C++ plugin; declaration shown without linkage wrapper. */
const MprPluginApiV1* mpr_get_api_v1(void);
```

**This is not a standalone compilable header.** The real `plugin_api.h` must add `<stdint.h>`, include guards and the `extern "C"` conditional wrapper.

### Validation before registering

1. `dlopen(path, RTLD_NOW | RTLD_LOCAL)` succeeds (own the resulting reference with RAII immediately).
2. `dlsym(handle, "mpr_get_api_v1")` succeeds using `dlerror` correctly.
3. Calling the entry returns a non-null pointer to a valid, readable static-lifetime API object.
4. `abi_version == MPR_ABI_V1` and `struct_size == sizeof(MprPluginApiV1)` for strict V1; no prefix-extension compatibility yet.
5. `name` is a valid, nonempty NUL-terminated string under the trusted-plugin contract.
6. `create`, `destroy`, and `transform` pointers are non-null.
7. Registry does not already contain this logical name.
8. Only **after** all conditions pass is registration committed.

A version and size check cannot make a forged/out-of-bounds pointer safe. v0.1 trusts plugin code and its pointer validity. The table and name storage must remain valid for the dynamic library's loaded lifetime, and their content must not change.

### ABI function behavior

- **Entry function:** returns a valid table pointer or null; must not propagate a C++ exception.
- **`create(out_instance)`:** `out_instance` must be non-null; on success returns `MPR_STATUS_OK` and a non-null handle; on failure returns an error code, writes null, and cleans all partially created resources.
- **`destroy(instance)`:** called exactly once by Runtime for a successfully created handle; performs plugin-owned cleanup and must not let an exception escape.
- **`transform(instance, input, out_result)`:** with valid arguments returns `MPR_STATUS_OK` and writes the result; on failure returns a nonzero status, and Runtime discards any temporary output.
- **Error codes:** status `0` means success; all other values are errors. Runtime retains the original plugin status in `PluginError.native_status`, including unknown codes. `native_status == 0` denotes errors generated by the Runtime, not the plugin.

C ABI compatibility here means an explicitly agreed platform/toolchain calling convention and data layout, not arbitrary-compiler interoperability or sandboxing.

## 8. Error model

```text
Plugin C ABI status --> Runtime maps to PluginError --> std::expected<T, PluginError>
Linux loader error --> Runtime maps to PluginError --> std::expected<T, PluginError>
```

- `load`, `create`, `unregister`, `transform`: use `Result` for **anticipated operational failures**.
- `contains`: uses `bool`.
- Keep categories machine-readable (`ErrorCode`) and diagnostics human-readable (`message`).
- Do not mix exception and status return for different anticipated failures of the *same* API.
- This is not a no-throw-everywhere policy. Construction of messages or containers may still throw `std::bad_alloc` and similar unexpected exceptions.
- Exceptions must not cross the C ABI boundary by contract.

## 9. Exception safety and commit protocols

### Loading and registration

```text
Acquire dlopen reference under RAII
  -> resolve entry + validate table
  -> prepare owned logical plugin name / LoadedPlugin
  -> check registration collision
  -> commit into Registry once preparation is complete
```

No failure before commit leaves a partial registry entry. ELF initialization side effects are explicitly not reversible by this guarantee.

### Instance creation and handoff

```text
Acquire shared LoadedPlugin lifetime token before create
  -> prepare a guard that knows destroy + keeps library alive
  -> call ABI create
  -> validate returned status and handle
  -> transfer to PluginInstance in a no-throw handoff (or equivalent rollback-safe transition)
```

From the instant a valid instance handle exists, a unique cleanup owner must exist. When `PluginInstance` is destroyed or overwritten, its opaque handle is released before the final library lifetime token. Moved-from states must not call `destroy` again.

## 10. Tests / acceptance gates

| Test | Contract verified |
|---|---|
| Valid plugin load and `transform(5)` | Basic execution and ABI link |
| Wrong path | `LibraryOpenFailed` and unchanged Registry |
| Missing entry point | `EntryPointMissing`, no leaked loader reference |
| Wrong V1 version / size / missing function | `IncompatibleAbi` or `InvalidPluginApi` |
| Duplicate name | `DuplicatePlugin`, first registration remains valid |
| Valid instance creation / destruction | Exactly one `destroy` per created handle |
| Plugin create failure after partial allocation | Plugin cleans partial resources; no Active instance escapes |
| Registry unregister + surviving instance call | Instance remains usable |
| Registry destruction before instance destruction | LoadedPlugin stays valid through instance cleanup |
| Move construction and move assignment | Unique instance ownership, old target cleaned once |
| Self-move and moved-from transform | Safe cleanup and `InvalidInstance` |
| Missing registration on `create`/`unregister` | `PluginNotFound` |
| Plugin transform error | `PluginExecutionFailed` with native status |
| `LD_DEBUG=libs` diagnostic experiment | Observe loader activity without assuming immediate unmapping |

For reliable destroy-count checks, prefer a test plugin exposing an explicit count/log sink or a test-only callback; do **not** treat a successful AddressSanitizer run as proof of every dynamic-loading lifecycle property.

## 11. Implementation milestones (independent coding)

### M0 — Project skeleton and toolchain

- CMake target for Runtime, Host demo, two test plugins, CTest.
- Require C++23 and verify `<expected>` compiles with installed standard library.
- Establish build-from-clean-directory workflow.

**Gate:** Clean configure/build/test works; C ABI public header compiles as both C and C++.

### M1 — C ABI contract and simple test plugin

- Author `plugin_api.h`; implement a small CPU plugin exporting one V1 table.
- Verify single exported entry via `nm -D`.
- Write positive and negative test plugin variants (bad version, absent entry, create failure).

**Gate:** ABI signatures and error conventions documented; test plugin output known.

### M2 — Internal `LoadedPlugin` (loading/validation only)

- Internal RAII ownership for the `dlopen` reference.
- Single `dlsym` lookup and ABI checks.
- Store validated function table and copied logical name.
- Translate expected loader/validation errors to `Result`.

**Gate:** Good plugin succeeds; invalid path/entry/version/size/function pointer fail cleanly; no registration logic yet.

### M3 — Move-only `PluginInstance`

- Privately create instances via LoadedPlugin factory with guarded ownership transfer.
- Implement instance destructor, move construction and move assignment.
- Implement transform and moved-from error handling.

**Gate:** Every successful instance is destroyed exactly once. Instance survives factory owner scope when a shared token remains.

### M4 — `PluginRegistry`

- Map logical name to `shared_ptr<LoadedPlugin>`.
- `load`, `create`, `unregister`, `contains`.
- Build registration transaction with no partial commit.

**Gate:** Duplicate/invalid registration does not change logical map; instance survives unregister and Registry destruction.

### M5 — Integration / diagnostics / release baseline

- CTest scenarios, sanitizer-friendly tests, error-status assertions, README, `docs/design.md`.
- Validate current test toolchain and document supported environments.

**Gate:** All required tests pass from a clean checkout; v0.1 limits are documented; no supported hot replacement claim.

## 12. Suggested repository layout

```text
mini-plugin-runtime/
├── CMakeLists.txt
├── include/mpr/
│   ├── plugin_api.h          # C ABI (stable contract)
│   ├── error.hpp             # PluginError, ErrorCode, Result
│   ├── plugin_instance.hpp   # public C++ API
│   └── plugin_registry.hpp   # public C++ API
├── src/
│   ├── loaded_plugin.hpp     # private implementation
│   ├── loaded_plugin.cpp
│   ├── plugin_instance.cpp
│   └── plugin_registry.cpp
├── plugins/
│   ├── cpu_plugin.c or .cpp
│   └── failing_plugin.c or .cpp
├── tests/
├── examples/
│   └── host.cpp
├── docs/
│   └── design.md
└── README.md
```

Avoid creating empty placeholder classes just to match the tree; introduce files as the corresponding milestone begins.

## 13. Definition of done

v0.1 is complete when a developer can build the project from a clean checkout, load one trusted V1 plugin, create and move instances, execute computation, receive structured expected-based errors, unregister a plugin while prior instances remain usable, and demonstrate normal cleanup and specified failure paths with automated tests. No concurrent use, hot reload, version coexistence, or security-isolation guarantees are asserted.

## 14. References for language and loader behavior

- C++23 `std::expected`: https://www.cppreference.com/cpp/utility/expected
- Linux `dlopen(3)` behavior: https://man7.org/linux/man-pages/man3/dlopen.3.html
