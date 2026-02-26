#!/bin/bash

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 获取进程信息
PID=$(pgrep kcp_server)

if [ -z "$PID" ]; then
    echo -e "${RED}❌ kcp_server process not found!${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Found kcp_server with PID: $PID${NC}"
echo "=========================================="

# 1. 基本进程信息
echo -e "${YELLOW}1. Basic Process Info:${NC}"
ps -p $PID -o pid,ppid,pcpu,pmem,stat,comm
echo ""

# 2. CPU使用详情
echo -e "${YELLOW}2. CPU Usage Details (top -p $PID):${NC}"
top -b -n 1 -p $PID | tail -n +7
echo ""

# 3. 线程级别CPU使用
echo -e "${YELLOW}3. Thread-level CPU Usage:${NC}"
top -H -b -n 1 -p $PID | tail -n +8 | head -10
echo ""

# 4. 系统调用统计（运行5秒）
echo -e "${YELLOW}4. System Call Statistics (5 seconds):${NC}"
strace -p $PID -c 2>/dev/null &
STRACE_PID=$!
sleep 5
kill $STRACE_PID 2>/dev/null
wait $STRACE_PID 2>/dev/null
echo ""

# 5. 查看网络相关系统调用
echo -e "${YELLOW}5. Network-related System Calls:${NC}"
strace -p $PID -e trace=network -f -c 2>/dev/null &
STRACE_PID=$!
sleep 3
kill $STRACE_PID 2>/dev/null
wait $STRACE_PID 2>/dev/null
echo ""

# 6. 打开的文件描述符
echo -e "${YELLOW}6. Open File Descriptors (first 20):${NC}"
lsof -p $PID 2>/dev/null | head -20
echo ""

# 7. 内存映射
echo -e "${YELLOW}7. Memory Maps:${NC}"
cat /proc/$PID/maps | head -10
echo ""

# 8. 当前系统调用（如果进程正在运行）
echo -e "${YELLOW}8. Current System Call (if running):${NC}"
if [ -d /proc/$PID ]; then
    cat /proc/$PID/stack 2>/dev/null | head -10
fi
echo ""

# 9. 使用 perf 分析（如果有权限）
echo -e "${YELLOW}9. Perf Top Analysis (5 seconds):${NC}"
if command -v perf &> /dev/null; then
    sudo perf top -p $PID -d 1 -n 20 2>/dev/null &
    PERF_PID=$!
    sleep 5
    kill $PERF_PID 2>/dev/null
else
    echo "perf not installed"
fi
echo ""

# 10. GDB 快速查看调用栈
echo -e "${YELLOW}10. Quick GDB Stack Trace:${NC}"
sudo gdb -p $PID -batch -ex "thread apply all bt" 2>/dev/null | head -50
echo ""

echo -e "${GREEN}==========================================${NC}"
echo -e "${YELLOW}Summary:${NC}"

# CPU使用率检查
CPU_USAGE=$(ps -p $PID -o %cpu --no-headers)
if (( $(echo "$CPU_USAGE > 90" | bc -l) )); then
    echo -e "${RED}⚠️  High CPU usage: $CPU_USAGE%${NC}"
else
    echo -e "${GREEN}✅ CPU usage normal: $CPU_USAGE%${NC}"
fi

# 线程数检查
THREAD_COUNT=$(ps -o nlwp $PID | tail -1 | tr -d ' ')
echo "Thread count: $THREAD_COUNT"

# 运行时间
RUNTIME=$(ps -o etime $PID | tail -1 | tr -d ' ')
echo "Runtime: $RUNTIME"

echo -e "${GREEN}==========================================${NC}"