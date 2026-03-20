本项目包含或者借鉴第三方 KCP (ikcp, kcp-csharp) 代码，详见 THIRD_PARTY_NOTICES。

# akcp

基于 Boost.Asio + KCP 的轻量封装库，提供面向业务的 `server/client/channel` 接口，支持连接管理、消息收发、超时回收与基础压测示例。

提供了CSharp的协议接口,可以和C++版本正常通信

## 功能概览

- 封装 KCP 会话生命周期（创建、收发、更新、释放）
- 基于事件循环的 UDP 收发与回调分发
- 提供服务端/客户端 API：`kcp::server`、`kcp::client`
- 内置 demo 与 stress test 工程，便于快速验证

## 目录结构

- `common/`：通用组件与 KCP 源码（`ikcp.c/.h`、协议与时间工具）
- `resource/`：核心库实现（server/client/channel/context/timer）
- `test/`：示例与测试（single_demo、multi_thread、stress_test）

## 编译安装

要求：

- CMake >= 3.10
- C++17 编译器
- Boost.Asio（Boost）
- jsoncpp（仅 `client_stress` 需要）

linux

```bash
mkdir build; cd build
cmake -S .. -B . -DAKCP_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=${/path/to/dir}
make; sudo make install
```

windows

```bash
mkdir build; cd build
cmake -S .. -B . -DCMAKE_TOOLCHAIN_FILE=${vcpkg-install-dir}/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DAKCP_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=${/path/to/dir}

cmake --build . --config Release
cmake --install . --config Release
```

## 快速运行

安装之后使用
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(demo_use_akcp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(akcp CONFIG REQUIRED)

add_executable(server main.cc)
target_link_libraries(server PRIVATE akcp::akcp)
```

## 测试

编译
```bash
mkdir build; cd build
cmake -S .. -B .
```

### 单连接示例

```bash
# 终端1
./build/test/single_demo/server

# 终端2
./build/test/single_demo/client
```

### 压测示例

```bash
# 终端1
./build/test/stress_test/server_stress 8080

# 终端2
./build/test/stress_test/client_stress 127.0.0.1 8080 100 30 60 128
```

参数含义：

`ip port clients duration_seconds req_per_client_per_sec packet_size`

## Socket 缓冲区说明

项目已在代码里设置 UDP 收发缓冲区（见 `common/common.hh` 与 `resource/io_socket.cc`）。
高压场景下建议同步调整系统上限，否则可能出现突发丢包。

Linux 示例：

```bash
sudo sysctl -w net.core.rmem_max=134217728
sudo sysctl -w net.core.rmem_default=134217728
sudo sysctl -w net.core.wmem_max=134217728
sudo sysctl -w net.core.wmem_default=134217728
```
