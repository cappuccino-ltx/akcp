This project includes or refers to third-party KCP (ikcp, kcp-csharp) code. See THIRD_PARTY_NOTICES for details.

# akcp

[中文版](./README_zh.md) | [English](./README.md)

---

A lightweight wrapper library based on **Boost.Asio** and **KCP**. It provides business-oriented `server/client/channel` interfaces, supporting connection management, message transceiving, timeout recycling, and comprehensive benchmarking examples.

The project provides C# protocol interfaces compatible with the C++ version for seamless cross-language communication.

## Features

- **Lifecycle Management**: Encapsulates KCP session creation, transceiving, updating, and releasing.
- **Event-Driven**: High-performance UDP I/O and callback distribution based on Boost.Asio.
- **Easy-to-use API**: Clean interfaces for `kcp::server` and `kcp::client`.
- **Latency/Throughput Trade-off**: Enabled low-delay mode by default. Provides interfaces to manually disable low-delay mode for higher PPS and BPS.
- **Buffer Pool**: Supports custom buffer pool settings to minimize memory fragmentation.
- **Linux Optimizations**: Includes batch sending optimizations and CPU affinity (core pinning) support.
- **Built-in Benchmarks**: Includes demos and stress testing tools for rapid verification.

## Directory Structure

- `resource/`: Core library implementation (server, client, channel, context, timer).
- `test/`: Examples and benchmarks (single_demo, multi_thread, stress_test).

## Build & Installation

Requirements:
- CMake >= 3.10
- C++17 Compiler
- Boost.Asio
- jsoncpp (required only for `client_stress`)

### Linux
```bash
mkdir build; cd build
cmake -S .. -B . -DAKCP_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=${/path/to/dir}
make; sudo make install
```

### Windows
```bash
mkdir build; cd build
cmake -S .. -B . -DCMAKE_TOOLCHAIN_FILE=${vcpkg-install-dir}/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DAKCP_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=${/path/to/dir}

cmake --build . --config Release
cmake --install . --config Release
```

## Quick Start

After installation, use in your `CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.10)
project(demo_use_akcp)

set(CMAKE_CXX_STANDARD 17)
find_package(akcp CONFIG REQUIRED)

add_executable(server main.cc)
target_link_libraries(server PRIVATE akcp::akcp)
```

## Testing

### Compilation
```bash
mkdir build; cd build
cmake -S .. -B .
make
```

### Single Connection Demo
```bash
# Terminal 1
./build/test/single_demo/server

# Terminal 2
./build/test/single_demo/client
```

### Benchmark Notes

The `test/` directory contains various test cases: [Single Demo](test/single_demo/), [Multi-threaded Server Sharding](test/multi_thread/), [Stress Test](test/stress_test/), [PPS/BPS/P99 Metrics](test/pps_bps_p99/), and [14-Hour Stability Test](test/balance/).

**Note on Performance**: In PPS and Stability tests, you can disable the low-delay mode on both server and client. This allows KCP to perform packet bundling (aggregation), resulting in significantly higher PPS and BPS. However, the latency will increase to approximately the value of your configured `interval` (KCP internal update clock).

### Benchmark Results

*Results may vary based on hardware and network conditions. All data below are based on local loopback tests to eliminate network bandwidth bottlenecks.*

**Environment:**
- CPU: 12 Cores / RAM: 16 GB
- OS: Ubuntu 22.04 LTS
- CPU Affinity: 5 threads for Server, 7 threads for Client.

#### 1. Low-Delay Mode Enabled
Scalability test from 1,000 to 11,000 concurrent clients.

**BPS & PPS**

![bps&pps](test/result/enable_low_delay/throughput_pps.png)

**Latency (P50, P99, P999)**

![latency](test/result/enable_low_delay/latency.png)

#### 2. Low-Delay Mode Disabled (Throughput Optimized)
Disabling low-delay mode triggers KCP's packet bundling mechanism.

**BPS & PPS**

![bps&pps](test/result/disable_low_delay/throughput_pps.png)

**Latency (P50, P99, P999)**

![latency](test/result/disable_low_delay/latency.png)

*Data Note: The server only echoes back the data sent by clients. All statistics represent application-layer data only (KCP overhead and handshakes excluded).*

#### 3. 14-Hour Stability Test
The server ran in low-delay mode for 14 hours with the following load:
- 3,000 persistent connections with continuous requests.
- 1,000 churn connections (disconnecting and reconnecting every 10 minutes).

System resources were monitored via `pidstat`:

![balance](test/result/stress_report.png)

*All benchmark charts are available in `test/result`.*

## Socket Buffer Configuration

The project configures UDP transceiving buffers in code (see `common/common.hh` and `resource/io_socket.cc`). Under high-load scenarios, it is highly recommended to increase the system limits to prevent burst packet loss.

**Linux Example:**
```bash
sudo sysctl -w net.core.rmem_max=134217728
sudo sysctl -w net.core.rmem_default=134217728
sudo sysctl -w net.core.wmem_max=134217728
sudo sysctl -w net.core.wmem_default=134217728
sudo sysctl -w net.core.netdev_max_backlog=10000
```