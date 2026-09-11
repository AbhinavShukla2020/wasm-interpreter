# miniwm — WebAssembly Interpreter in C++17

miniwm is a small WebAssembly binary decoder and stack-machine interpreter. It
parses modules without WABT at runtime, validates section boundaries and indices,
links structured control flow, and executes a useful MVP integer subset with
checked linear memory.

## Build and run

Requirements are a C++17 compiler, CMake 3.20+, GoogleTest, and optionally WABT
for compiling `.wat` samples and converting official spec tests.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure

wat2wasm samples/add.wat -o build/add.wasm
./build/miniwm build/add.wasm add 19 23
# 42
```

## Implemented

- Strict magic/version and ordered-section parsing.
- Type, function, memory, export, code, and active data sections.
- Signed/unsigned LEB128 with truncation and overflow checks.
- i32 constants, locals, comparisons, arithmetic, bitwise operations, shifts,
  direct calls, selection, structured void blocks, loops, branches, and returns.
- Checked i32 loads/stores plus `memory.size` and `memory.grow`.
- Traps for unreachable, divide-by-zero, integer overflow, stack errors, invalid
  branches, and out-of-bounds memory.
- GoogleTest unit tests and a WABT JSON bridge for supported official assertions.

## Scope

This is an educational interpreter rather than a complete WebAssembly runtime.
It currently excludes imports, globals, tables, indirect calls, floating point,
SIMD, exceptions, WASI, and typed block results. Those omissions mean arbitrary
Clang output and CoreMark are not yet expected to execute.

The documentation describes how to extend the conformance runner and compare
against wasm3 without treating skipped spec assertions as passes or publishing
unmeasured performance figures.

