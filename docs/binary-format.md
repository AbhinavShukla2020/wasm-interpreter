# Binary decoder

A WebAssembly file begins with a four-byte magic number and version, followed by
ordered sections. The decoder keeps a strict slice for every section and
function body. A parser must consume the slice exactly, which catches malformed
lengths and prevents one section from reading bytes owned by the next.

Unsigned and signed LEB128 readers reject truncation and overflow. Vector lengths,
indices, and section sizes use unsigned LEB128; integer constants use signed
LEB128. Function bodies are decoded into typed instructions before execution.
This makes unsupported opcodes fail during module loading rather than halfway
through a hot function.

The control-linking pass matches `block`, `loop`, `if`, `else`, and `end` once.
The interpreter can then branch directly to an instruction index instead of
rescanning bytecode on every loop iteration.

