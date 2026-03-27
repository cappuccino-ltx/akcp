import pandas as pd
import matplotlib.pyplot as plt
import re
import argparse
import os

def clean_pidstat_to_df(input_log_path):
    cpu_records = []
    mem_records = []
    
    time_regex = re.compile(r'^\d{2}:\d{2}:\d{2}')

    if not os.path.exists(input_log_path):
        print(f"错误: 找不到文件 {input_log_path}")
        return None

    # 状态机：记录当前行属于哪种统计块
    is_cpu_block = False
    is_mem_block = False

    with open(input_log_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            line = line.strip()
            if not line: continue

            # 1. 检测表头，切换状态
            if "%CPU" in line:
                is_cpu_block = True
                is_mem_block = False
                continue
            if "VSZ" in line or "RSS" in line:
                is_mem_block = True
                is_cpu_block = False
                continue

            # 2. 匹配数据行 (必须以时间戳开头)
            if time_regex.match(line):
                parts = line.split()
                if len(parts) < 8: continue
                
                try:
                    # 只要包含我们要的进程名
                    if "server_balance" in parts[-1]:
                        if is_cpu_block:
                            # CPU 行：时间 UID PID %usr %system %guest %wait %CPU CPU Command
                            # %CPU 通常是倒数第 3 列，%system 是倒数第 6 列
                            cpu_records.append({
                                'Time': parts[0],
                                'CPU_Total': float(parts[-3]),
                                'System_Usage': float(parts[-6])
                            })
                        elif is_mem_block:
                            # 内存行：时间 UID PID minflt/s majflt/s VSZ RSS %MEM Command
                            # RSS 是倒数第 3 列，%MEM 是倒数第 2 列
                            mem_records.append({
                                'Time': parts[0],
                                'RSS_KB': int(parts[-3]),
                                'MEM_Percent': float(parts[-2])
                            })
                except (ValueError, IndexError):
                    continue

    print(f"识别到 CPU 记录: {len(cpu_records)} 条")
    print(f"识别到 内存 记录: {len(mem_records)} 条")

    df_cpu = pd.DataFrame(cpu_records)
    df_mem = pd.DataFrame(mem_records)

    if df_cpu.empty or df_mem.empty:
        print("警告: 依然未能提取到完整的双向数据。")
        return None

    # 按时间对齐
    df_final = pd.merge(df_cpu, df_mem, on='Time', how='inner')
    # 去重
    df_final = df_final.drop_duplicates(subset=['Time']).reset_index(drop=True)
    
    return df_final

def plot_stress_test_analysis(df, save_path='stress_report.png'):
    """
    工具函数 2: 将清洗后的 DataFrame 生成双轴压力分析图
    """
    if df is None or df.empty:
        return

    # 设置画布大小
    fig, ax1 = plt.subplots(figsize=(14, 8))

    # --- 绘制 CPU 曲线 (左轴) ---
    color_cpu = '#e74c3c' # 红色
    ax1.set_xlabel('Timeline (Sampled Index)', fontsize=12)
    ax1.set_ylabel('CPU Usage Total (%)', color=color_cpu, fontsize=12, fontweight='bold')
    ax1.plot(df.index, df['CPU_Total'], color=color_cpu, label='CPU Total %', linewidth=1, alpha=0.8)
    ax1.tick_params(axis='y', labelcolor=color_cpu)
    ax1.grid(True, linestyle='--', alpha=0.5)

    # --- 绘制 内存 曲线 (右轴) ---
    ax2 = ax1.twinx()
    color_mem = '#3498db' # 蓝色
    ax2.set_ylabel('Memory RSS (KB)', color=color_mem, fontsize=12, fontweight='bold')
    ax2.plot(df.index, df['RSS_KB'], color=color_mem, label='Memory RSS (KB)', linewidth=2.5)
    ax2.tick_params(axis='y', labelcolor=color_mem)

    # 装饰图表
    plt.title('KCP Server 24H Stability & Stress Analysis', fontsize=16, pad=20)
    
    # 合并图例
    lines, labels = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(lines + lines2, labels + labels2, loc='upper left', frameon=True, shadow=True)

    # 自动调整布局并保存
    plt.tight_layout()
    plt.savefig(save_path, dpi=300)
    print(f"分析报告已生成: {save_path}")
    plt.show()

# --- 使用示例 ---
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Performance Plot Tool")
    parser.add_argument("file", help="输入CSV文件路径")
    args = parser.parse_args()
    # 1. 清洗数据
    cleaned_df = clean_pidstat_to_df(args.file)
    
    if cleaned_df is not None:
        # 2. 导出 CSV 备份
        cleaned_df.to_csv('cleaned_performance.csv', index=False)
        # 3. 生成图表
        plot_stress_test_analysis(cleaned_df)