import pandas as pd
import matplotlib.pyplot as plt
import argparse
import os

def plot_performance(file_path, output_dir):
    # ====== 读取数据 ======
    df = pd.read_csv(file_path)

    # 创建输出目录
    os.makedirs(output_dir, exist_ok=True)

    # ====== 图1：吞吐 + PPS ======
    fig, ax1 = plt.subplots()

    ax1.set_xlabel("Clients")
    ax1.set_ylabel("Throughput (MB/s)")
    ax1.plot(df["client"], df["bps(MB/s)"], marker='o', label="Throughput")

    ax2 = ax1.twinx()
    ax2.set_ylabel("PPS (k/s)")
    ax2.plot(df["client"], df["pps(k/s)"], marker='x', linestyle='--', label="PPS")

    plt.title("Throughput & PPS vs Clients")
    plt.grid()

    fig.savefig(os.path.join(output_dir, "throughput_pps.png"))
    plt.close(fig)

    # ====== 图2：延迟 ======
    plt.figure()

    plt.plot(df["client"], df["p50(ms)"], marker='o', label="p50")
    plt.plot(df["client"], df["p99(ms)"], marker='x', label="p99")
    plt.plot(df["client"], df["p999(ms)"], marker='s', label="p999")

    plt.xlabel("Clients")
    plt.ylabel("Latency (ms)")
    plt.title("Latency Percentiles vs Clients")
    plt.legend()
    plt.grid()

    plt.savefig(os.path.join(output_dir, "latency.png"))
    plt.close()

    # ====== 图3：吞吐 vs 尾延迟 ======
    plt.figure()

    plt.plot(df["bps(MB/s)"], df["p99(ms)"], marker='o')

    plt.xlabel("Throughput (MB/s)")
    plt.ylabel("p99 Latency (ms)")
    plt.title("Throughput vs Tail Latency")
    plt.grid()

    plt.savefig(os.path.join(output_dir, "throughput_vs_latency.png"))
    plt.close()

    print(f"✅ 图已生成到目录: {output_dir}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Performance Plot Tool")
    parser.add_argument("file", help="输入CSV文件路径")
    parser.add_argument("-o", "--output", default="output", help="输出目录")

    args = parser.parse_args()

    plot_performance(args.file, args.output)