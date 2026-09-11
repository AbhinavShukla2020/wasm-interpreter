# Benchmarking

`benchmarks/benchmark.py` compares process-level latency for the same module and
export under miniwm and wasm3. Process startup can dominate a short function, so
use a long-running benchmark or add an in-process repetition export before
interpreting the ratio.

For CoreMark, compile the portable C sources with a freestanding wasm32 toolchain
and provide the small timing/printing imports it expects. Record compiler flags,
CoreMark iteration count, runtime versions, CPU model, and whether validation and
module loading are included. Run at least five samples with fixed CPU frequency.

This repository does not include a precomputed “45% of wasm3” value. The current
MVP subset must gain imported functions and typed control blocks before standard
CoreMark builds will run. The benchmark harness is present so that performance
work can be measured as those features land.

