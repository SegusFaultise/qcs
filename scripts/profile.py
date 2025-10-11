import subprocess
import os
import re
import shutil
import argparse
import pandas as pd
import matplotlib.pyplot as plt

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
SRC_DIR = os.path.join(ROOT_DIR, "src")
BUILD_DIR = os.path.join(ROOT_DIR, "build")
PROFILE_DIR = os.path.join(ROOT_DIR, "profile")
EXECUTABLE_NAME = "qcs"


def check_dependencies():
    """Checks if required system tools are installed."""
    print("--- Checking for dependencies ---")
    dependencies = ['gcc', '/usr/bin/time']
    for dep in dependencies:
        if not shutil.which(dep):
            print(f"❌ Error: '{dep}' not found. Please install it.")
            exit(1)
    print("✅ All system dependencies found.\n")


def compile_program(is_parallel=False):
    """Compiles all C source files, placing the executable in the build directory."""
    print(f"--- Compiling Program (Parallel Mode: {is_parallel}) ---")
    if not os.path.exists(BUILD_DIR):
        os.makedirs(BUILD_DIR)

    source_files = [os.path.join(SRC_DIR, f)
                    for f in os.listdir(SRC_DIR) if f.endswith('.c')]
    executable_path = os.path.join(BUILD_DIR, EXECUTABLE_NAME)

    compile_command = [
        'gcc', '-std=c89', '-g', '-O3', '-o', executable_path
    ] + source_files + ['-lm']

    if is_parallel:
        compile_command.append('-fopenmp')
        print("   Using '-fopenmp' flag.")

    print(f"   Command: {' '.join(compile_command)}")
    result = subprocess.run(compile_command, capture_output=True, text=True)

    if result.returncode != 0:
        print("❌ Compilation Failed!")
        print(result.stderr)
        exit(1)

    print("✅ Compilation successful.\n")
    return executable_path


def parse_time_output(stderr_str):
    """Parses the verbose output of /usr/bin/time."""
    max_ram_kb = re.search(
        r'Maximum resident set size \(kbytes\): (\d+)', stderr_str)
    user_time = re.search(r'User time \(seconds\): (\d+\.\d+)', stderr_str)

    return {
        'max_ram_mb': float(max_ram_kb.group(1)) / 1024 if max_ram_kb else 0,
        'user_time_sec': float(user_time.group(1)) if user_time else 0
    }


def run_benchmark(executable_path, num_qubits):
    """Runs the benchmark and returns performance data."""
    print(f"--- Running benchmark for {num_qubits} qubits ---")
    command = ['/usr/bin/time', '-v', executable_path, str(num_qubits)]

    try:
        result = subprocess.run(
            command, capture_output=True, text=True, check=True, cwd=ROOT_DIR)
        print(result.stdout)

        perf_data = parse_time_output(result.stderr)
        perf_data['qubits'] = num_qubits

        exec_time_match = re.search(
            r'Execution time: ([\d.]+) seconds', result.stdout)
        perf_data['exec_time_sec'] = float(exec_time_match.group(
            1)) if exec_time_match else perf_data['user_time_sec']

        print(f"   -> Time: {perf_data['exec_time_sec']              :.2f}s, Peak RAM: {perf_data['max_ram_mb']:.2f} MB")
        return perf_data

    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print(f"❌ Benchmark failed for {num_qubits} qubits.")
        if hasattr(e, 'stderr'):
            print(e.stderr)
        return None


def generate_reports(results_df, output_dir):
    """Generates and saves plots and a CSV summary to the specified directory."""
    print(f"\n--- Generating Reports in '{output_dir}' ---")
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    summary_csv_path = os.path.join(output_dir, 'benchmark_summary.csv')
    results_df.to_csv(summary_csv_path, index=False)
    print(f"   -> Saved summary data to '{summary_csv_path}'")

    plt.style.use('seaborn-v0_8-whitegrid')

    fig1, ax1 = plt.subplots(figsize=(10, 6))
    ax1.plot(results_df['qubits'], results_df['exec_time_sec'],
             marker='o', linestyle='-', color='b')
    ax1.set_title('Execution Time vs. Number of Qubits', fontsize=16)
    ax1.set_xlabel('Number of Qubits (N)', fontsize=12)
    ax1.set_ylabel('Execution Time (seconds)', fontsize=12)
    ax1.set_xticks(results_df['qubits'])
    ax1.set_yscale('log')
    ax1.get_yaxis().set_major_formatter(plt.ScalarFormatter())
    time_plot_path = os.path.join(output_dir, 'plot_time_vs_qubits.png')
    fig1.savefig(time_plot_path)
    print(f"   -> Saved time scaling plot to '{time_plot_path}'")

    fig2, ax2 = plt.subplots(figsize=(10, 6))
    ax2.plot(results_df['qubits'], results_df['max_ram_mb'],
             marker='s', linestyle='--', color='r')
    ax2.set_title('Peak RAM Usage vs. Number of Qubits', fontsize=16)
    ax2.set_xlabel('Number of Qubits (N)', fontsize=12)
    ax2.set_ylabel('Peak RAM Usage (MB)', fontsize=12)
    ax2.set_xticks(results_df['qubits'])
    ax2.set_yscale('log')
    ax2.get_yaxis().set_major_formatter(plt.ScalarFormatter())
    ram_plot_path = os.path.join(output_dir, 'plot_ram_vs_qubits.png')
    fig2.savefig(ram_plot_path)
    print(f"   -> Saved RAM scaling plot to '{ram_plot_path}'")

    print("✅ Report generation complete.")


def main():
    """Main function to orchestrate profiling."""
    parser = argparse.ArgumentParser(
        description="Compile, run, and profile the QCS benchmark.")
    parser.add_argument('--qubits', nargs='+', type=int, default=[18, 20, 22, 24],
                        help='A space-separated list of qubit numbers to test.')
    parser.add_argument('--parallel', action='store_true',
                        help='Compile with OpenMP for parallel execution.')
    args = parser.parse_args()

    check_dependencies()
    executable = compile_program(is_parallel=args.parallel)

    results = []
    for q in sorted(args.qubits):
        result = run_benchmark(executable, q)
        if result:
            results.append(result)
        else:
            print(f"Skipping report generation due to benchmark failure at {
                  q} qubits.")
            return

    if results:
        results_df = pd.DataFrame(results)
        print("\n--- Benchmark Summary ---")
        print(results_df.to_string(index=False))
        generate_reports(results_df, PROFILE_DIR)


if __name__ == "__main__":
    main()
