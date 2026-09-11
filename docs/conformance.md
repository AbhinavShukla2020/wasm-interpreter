# Conformance workflow

The WebAssembly specification repository publishes `.wast` suites containing
modules plus assertions. WABT's `wast2json` tool turns a suite into individual
`.wasm` files and a JSON command manifest. `tests/run_spec.py` executes the i32
assertions supported by miniwm and reports passed, failed, and explicitly skipped
commands.

```bash
python3 tests/run_spec.py path/to/spec/test/core/i32.wast
```

Skipped assertions are not passes. Keep the three counts together when tracking
progress. Growing conformance requires adding imports, globals, tables, indirect
calls, numeric conversions, floating point, bulk memory, and typed block results.

GoogleTest covers decoder boundaries and a hand-encoded add module independently
of WABT. This prevents the text-to-binary tool from becoming the only path into
the parser.

