from __future__ import annotations

import argparse
import json
import statistics
import subprocess
import time


def measure(command: list[str], iterations: int) -> dict[str, float]:
    samples = []
    for _ in range(5):
        subprocess.run(command, check=True, stdout=subprocess.DEVNULL)
    for _ in range(iterations):
        started = time.perf_counter_ns()
        subprocess.run(command, check=True, stdout=subprocess.DEVNULL)
        samples.append((time.perf_counter_ns() - started) / 1e6)
    return {
        "median_ms": statistics.median(samples),
        "mean_ms": statistics.mean(samples),
        "min_ms": min(samples),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Compare miniwm and wasm3 process-level latency")
    parser.add_argument("module")
    parser.add_argument("export")
    parser.add_argument("args", nargs="*")
    parser.add_argument("--iterations", type=int, default=50)
    parser.add_argument("--miniwm", default="build/miniwm")
    parser.add_argument("--wasm3", default="wasm3")
    options = parser.parse_args()
    commands = {
        "miniwm": [options.miniwm, options.module, options.export, *options.args],
        "wasm3": [options.wasm3, "--func", options.export, options.module, *options.args],
    }
    print(json.dumps({name: measure(command, options.iterations) for name, command in commands.items()}, indent=2))


if __name__ == "__main__":
    main()

