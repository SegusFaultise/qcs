import subprocess
import os
import re
import shutil
import argparse
import pandas as pd
import matplotlib.pyplot as plt
import time

from tqdm.auto import tqdm


ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
SRC_DIR = os.path.join(ROOT_DIR, "src")
BUILD_DIR = os.path.join(ROOT_DIR, "build")
PROFILE_DIR = os.path.join(ROOT_DIR, "profile")
EXECUTABLE_NAME = "qcs"


def log_header(message):
    print(f"\n[{message}]")


def log_error(message):
    print(f"ERROR: {message}")


def check_dependencies():
    dependencies = ['gcc', '/usr/bin/time', 'pidstat', 'pgrep']
    for dep in dependencies:
        if not shutil.which(dep):
            log_error(f"'{dep}' not found. Please install it.")
            return False
    return True


def compile_program(is_parallel: bool = False) -> str:
    os.makedirs(BUILD_DIR, exist_ok=True)
    source_files = [os.path.join(SRC_DIR, f)
                    for f in os.listdir(SRC_DIR) if f.endswith('.c')]

    executable_path = os.path.join(BUILD_DIR, EXECUTABLE_NAME)
    compile_command = ['gcc', '-std=c89', '-g', '-O3',
                       '-o', executable_path] + source_files + ['-lm']
    if is_parallel:
        compile_command.append('-fopenmp')

    result = subprocess.run(compile_command, capture_output=True, text=True)

    if result.returncode != 0:
        log_error("Compilation Failed!")
        print(result.stderr)
        return None

    return executable_path


def parse_time_output(stderr_str: str) -> dict:
    max_ram_kb = re.search(
        r'Maximum resident set size \(kbytes\): (\d+)', stderr_str)
    user_time = re.search(r'User time \(seconds\): (\d+\.\d+)', stderr_str)

    return {
        'max_ram_mb': float(max_ram_kb.group(1)) / 1024 if max_ram_kb else 0,
        'user_time_sec': float(user_time.group(1)) if user_time else 0
    }


def run_benchmark_with_monitoring(executable_path: str, num_qubits: int, output_dir: str) -> dict:
    cpu_log_path = os.path.join(output_dir, f'cpu_usage_{
                                num_qubits}_qubits.log')
    command = ['/usr/bin/time', '-v', executable_path, str(num_qubits)]

    try:
        benchmark_process = subprocess.Popen(
            command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=ROOT_DIR)

        parent_pid = benchmark_process.pid
        child_pid = None

        for _ in range(20):
            pgrep_result = subprocess.run(
                ['pgrep', '-P', str(parent_pid)], capture_output=True, text=True)

            child_pid_str = pgrep_result.stdout.strip()

            if child_pid_str:
                child_pid = int(child_pid_str)
                break

            time.sleep(0.1)

        monitor_pid = child_pid if child_pid else parent_pid
        pidstat_process = subprocess.Popen(
            ['pidstat', '-p', str(monitor_pid), '-u', '1'], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)

        stdout, stderr = benchmark_process.communicate()
        pidstat_output, _ = pidstat_process.communicate()

        with open(cpu_log_path, 'w') as f:
            f.write(pidstat_output)

        if benchmark_process.returncode != 0:
            raise subprocess.CalledProcessError(
                benchmark_process.returncode, command, stdout, stderr)

        perf_data = parse_time_output(stderr)
        perf_data['qubits'] = num_qubits
        exec_time_match = re.search(
            r'Execution time: ([\d.]+) seconds', stdout)
        perf_data['exec_time_sec'] = float(exec_time_match.group(
            1)) if exec_time_match else perf_data['user_time_sec']

        return perf_data

    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        log_error(f"Benchmark failed for {num_qubits} qubits.")

        if hasattr(e, 'stderr'):
            print(e.stderr)

        return None


def generate_reports(results_df: pd.DataFrame, output_dir: str):
    log_header("GENERATING REPORTS")
    os.makedirs(output_dir, exist_ok=True)

    results_df.to_csv(os.path.join(
        output_dir, 'benchmark_summary.csv'), index=False)

    plt.style.use('seaborn-v0_8-whitegrid')

    for y_axis, title, y_label, marker, color, filename in [
        ('exec_time_sec', 'Execution Time vs. Number of Qubits',
         'Execution Time (seconds, log scale)', 'o', 'b', 'plot_time_vs_qubits.png'),
        ('max_ram_mb', 'Peak RAM Usage vs. Number of Qubits',
         'Peak RAM Usage (MB, log scale)', 's', 'r', 'plot_ram_vs_qubits.png')
    ]:
        fig, ax = plt.subplots(figsize=(10, 6))
        ax.plot(results_df['qubits'], results_df[y_axis],
                marker=marker, linestyle='-', color=color)
        ax.set_title(title, fontsize=16, weight='bold')
        ax.set_xlabel('Number of Qubits (N)', fontsize=12)
        ax.set_ylabel(y_label, fontsize=12)
        ax.set_xticks(results_df['qubits'])
        ax.set_yscale('log')
        ax.grid(True, which='both', linestyle='--', linewidth=0.5)
        ax.get_yaxis().set_major_formatter(plt.ScalarFormatter())
        fig.savefig(os.path.join(output_dir, filename))
        plt.close(fig)


def main():
    parser = argparse.ArgumentParser(
        description="Compile, run, and profile the QCS benchmark.")
    parser.add_argument('--qubits', nargs='+', type=int, default=[
                        20, 22, 24], help='A space-separated list of qubit numbers to test.')
    parser.add_argument('--parallel', action='store_true',
                        help='Compile with OpenMP for parallel execution.')
    args = parser.parse_args()

    if not check_dependencies():
        exit(1)

    executable = compile_program(is_parallel=args.parallel)

    if not executable:
        exit(1)

    results = []
    qubit_runs = sorted(args.qubits)
    log_header("RUNNING BENCHMARKS")

    bar_format = "{desc:<25}{percentage:3.0f}%|{bar:30}| {n_fmt}/{total_fmt} [{elapsed}<{remaining} {postfix}]"

    with tqdm(qubit_runs, desc="Initializing...", bar_format=bar_format, unit="run") as pbar:
        for q in pbar:
            pbar.set_description(f"Benchmarking ({q} qubits)")
            result = run_benchmark_with_monitoring(executable, q, PROFILE_DIR)

            if result:
                results.append(result)
                pbar.set_postfix(time=f"{result['exec_time_sec']:.2f}s", ram=f"{
                                 result['max_ram_mb']:.0f}MB")
            else:
                log_error(f"Stopping benchmarks due to failure at {q} qubits.")
                break

    if results:
        results_df = pd.DataFrame(results)
        log_header("BENCHMARK SUMMARY")
        print(results_df.to_string(index=False))
        generate_reports(results_df, PROFILE_DIR)

        log_header("REPORTS GENERATED")
        for filename in sorted(os.listdir(PROFILE_DIR)):
            print(f"  - {os.path.join(PROFILE_DIR, filename)}")


if __name__ == "__main__":
    main()
