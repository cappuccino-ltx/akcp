本项目包含或者借鉴第三方 KCP (ikcp, kcp-csharp) 代码，详见 THIRD_PARTY_NOTICES。

# akcp

基于 Boost.Asio + KCP 的轻量封装库，提供面向业务的 `server/client/channel` 接口，支持连接管理、消息收发、超时回收与基础压测示例。

提供了CSharp的协议接口,可以和C++版本正常通信

## 功能概览

- 封装 KCP 会话生命周期（创建、收发、更新、释放）
- 基于事件循环的 UDP 收发与回调分发
- 提供服务端/客户端 API：`kcp::server`、`kcp::client`
- 内置 demo 与 stress test 工程，便于快速验证
- 提供了 自定义 buffer pool 的设置接口
- 内部对 linux 环境下的发送进行了批量发送优化
- 内部对 linux 环境下的绑核操作进行了支持

## 目录结构

- `resource/`：核心库实现（server/client/channel/context/timer）
- `test/`：示例与测试（single_demo、multi_thread、stress_test）

## 编译安装

要求：

- CMake >= 3.10
- C++11 编译器
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

### 测试数据示例:
测试环境:

- 机器配置: 12 核心 16 GB 内存
- 操作系统: ubuntu 22.04 STL
- 单机测试( 为了忽略带宽的影响 )
- 客户端和服务端都进行和绑核操作(5 个服务端线程, 6 个客户端线程)

因为经过测试 单个核心跑 1200 个客户端的首发逻辑,cpu 占用就到 95 左右了,

测试命令:
```bash
# server
./server_stress 8080
#client
./client_stress 127.0.0.1 8080 7000 20 60 128
```

参数说明: 
- 客户端数量: 7000
- 单个客户端每秒发送 60 次消息
- 单个消息长度: 128 字节
- 客户端持续请求 20s

客户端线程 cpu 占用比例: 平均98%
服务器线程 cpu 占用比例: 平均65%

```json
{
    "1.settings" : 
    {
        "client number(preset)" : 7000,
        "client number(success)" : 7000,
        "package size" : 128,
        "request rate(/s)" : 60,
        "test time" : 20,
        "total speed" : 420000
    },
    "2.client result" : 
    {
        "bps" : 
        {
            "server rx(b/s)" : 52545171,
            "server tx(b/s)" : 52545171,
            "total(/s)" : 105090342
        },
        "loss" : 0,
        "pps" : 
        {
            "server rx(/s)" : 410509,
            "server tx(/s)" : 410509,
            "total(/s)" : 821018
        }
    }
}

```

数据说明:

- 测试服务器中,只对客户端发送的数据进行回显.
- 其中, 所有的数据均是应用层数据, 未统计 kcp 前置握手和 kcp 的心跳包和 ack 包.



## Socket 缓冲区说明

项目已在代码里设置 UDP 收发缓冲区（见 `common/common.hh` 与 `resource/io_socket.cc`）。
高压场景下建议同步调整系统上限，否则可能出现突发丢包。

Linux 示例：

```bash
sudo sysctl -w net.core.rmem_max=134217728
sudo sysctl -w net.core.rmem_default=134217728
sudo sysctl -w net.core.wmem_max=134217728
sudo sysctl -w net.core.wmem_default=134217728
sudo sysctl -w  net.core.netdev_max_backlog=10000
```
