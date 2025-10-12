import pandas as pd
import matplotlib.pyplot as plt
import os
import re


ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
PROFILE_DIR = os.path.join(ROOT_DIR, "profile")


def generate_cpu_plot(log_file_path: str):
    """
    Reads a single pidstat log file and generates a CPU usage plot.
    """
    if not os.path.exists(log_file_path):
        print(f"ERROR: Log file not found at '{log_file_path}'")
        return

    qubits_match = re.search(r'cpu_usage_(\d+)_qubits\.log', log_file_path)
    qubits_title = f"({qubits_match.group(1)} Qubits)" if qubits_match else ""

    print(f"--> Processing: {os.path.basename(log_file_path)}")

    try:
        header_index = 0

        with open(log_file_path, 'r') as f:
            for i, line in enumerate(f):
                if 'PID' in line and '%usr' in line:
                    header_index = i
                    break

        cpu_data = pd.read_csv(
            log_file_path,
            sep=r'\s+',
            skiprows=header_index,
            usecols=['%usr'],
            on_bad_lines='skip'
        )

        if cpu_data.empty:
            print("    WARNING: No data found. Benchmark may have finished too quickly.")
            return

        plt.style.use('seaborn-v0_8-whitegrid')
        fig, ax = plt.subplots(figsize=(10, 6))
        ax.plot(cpu_data.index, cpu_data['%usr'], linestyle='-', color='green')
        ax.set_title(f'CPU Usage Over Time {
                     qubits_title}', fontsize=16, weight='bold')
        ax.set_xlabel('Time (seconds)', fontsize=12)
        ax.set_ylabel('CPU Usage (%)', fontsize=12)
        ax.set_ylim(0, 105)
        ax.grid(True, which='both', linestyle='--', linewidth=0.5)

        output_filename = os.path.basename(
            log_file_path).replace('.log', '.png')

        output_path = os.path.join(os.path.dirname(
            log_file_path), f"plot_{output_filename}")

        fig.savefig(output_path)
        print(f" Plot saved to: {output_path}")
        plt.close(fig)

    except (pd.errors.EmptyDataError, ValueError, KeyError) as e:
        print(
            f"    ERROR: Could not parse file. It may be malformed. Details: {e}")


def main():
    """
    Finds all cpu_usage log files in the profile directory and generates plots for them.
    """
    print(f"\n[SCANNING FOR CPU LOGS IN '{PROFILE_DIR}']")

    log_files = [f for f in os.listdir(PROFILE_DIR) if f.startswith(
        'cpu_usage_') and f.endswith('.log')]

    if not log_files:
        print("No CPU log files found to plot.")
        return

    print(f"Found {len(log_files)} log file(s) to process.")
    for log_file in sorted(log_files):
        generate_cpu_plot(os.path.join(PROFILE_DIR, log_file))


if __name__ == "__main__":
    main()
