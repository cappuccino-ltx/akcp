import time
import psutil

def find_pids_by_name(name):
    pids = []
    for proc in psutil.process_iter(['pid', 'name']):
        if proc.info['name'] == name:
            pids.append(proc.info['pid'])
    return pids

PID = find_pids_by_name("server_stress")[0]
DURATION = 60  # 测试持续时间（秒）
INTERVAL = 1.0  # 采样间隔（秒）

p = psutil.Process(PID)

# 预热一次（很重要）
p.cpu_percent(interval=None)

cpu_values = []
mem_values = []

start = time.time()

while time.time() - start < DURATION:
    cpu = p.cpu_percent(interval=INTERVAL)  # 这1秒内CPU%
    mem = p.memory_info().rss / (1024 * 1024)  # 转MB
    
    cpu_values.append(cpu)
    mem_values.append(mem)

# 统计
cpu_avg = sum(cpu_values) / len(cpu_values)
cpu_max = max(cpu_values)

mem_avg = sum(mem_values) / len(mem_values)
mem_max = max(mem_values)

print("===== Resource Summary =====")
print(f"CPU Avg: {cpu_avg:.2f}%")
print(f"CPU Max: {cpu_max:.2f}%")
print(f"Memory Avg: {mem_avg:.2f} MB")
print(f"Memory Max: {mem_max:.2f} MB")