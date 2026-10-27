# 构建说明

项目目前面向 Linux，需要 CMake 3.20 或更新版本，以及支持 C++23
和 `std::expected` 的编译器与标准库。配置阶段会检查 `std::expected`。

## 配置和构建

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

当前 C++ 源文件尚未实现，`examples/host.cpp` 缺少 `main()`，因此完整
构建会在宿主程序链接阶段失败。库和插件虽然可以生成，仍只是空的构建产物，
不具备插件加载或执行功能。

开发运行时库和插件时，可以先关闭宿主示例：

```sh
cmake -S . -B build -DTPR_BUILD_EXAMPLES=OFF
cmake --build build --parallel
```

宿主实现完成后，用 `-DTPR_BUILD_EXAMPLES=ON` 重新启用。
`TPR_BUILD_PLUGINS` 控制是否构建示例插件，默认也是 `ON`。

## 构建目标

| 目标 | 默认产物 | 用途 |
| --- | --- | --- |
| `tpr` | `build/src/libtpr.a` | 运行时静态库，提供 `tpr::tpr` 别名 |
| `host` | `build/examples/host` | 链接运行时库的宿主示例 |
| `plugin` | `build/plugins/libplugin.so` | 由运行时动态加载的示例插件 |
| `failing_plugin` | `build/plugins/libfailing_plugin.so` | 用于失败场景的示例插件 |

这些路径适用于默认的单配置生成器。可用
`cmake --build build --target tpr` 单独构建某个目标。

运行时库公开 `include/` 头文件路径及 C++23 要求，并链接系统动态加载库。
插件使用 `MODULE` 类型，通过 `dlopen()` 加载，不需要链接运行时库。

Makefile 和 Ninja 生成器还会生成 `build/compile_commands.json`，供编辑器使用。

## 测试

`BUILD_TESTING` 默认开启，可用 `-DBUILD_TESTING=OFF` 关闭。
当前基础测试验证 `Result<T>` / `Result<void>` 的成功与失败状态，以及
`PluginError` 的错误码、消息、默认状态码和插件原生状态码保留。
测试不依赖第三方框架，Debug 和 Release 构建都会执行检查。
插件运行时尚未实现，因此暂不覆盖动态加载和实例生命周期。

```sh
cmake -S . -B build -DTPR_BUILD_EXAMPLES=OFF -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

也可以仅构建测试目标：`cmake --build build --target error_test`。
