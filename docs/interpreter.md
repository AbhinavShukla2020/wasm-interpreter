# Interpreter design

Each function invocation owns locals, an operand stack, and a control stack.
Arguments occupy the first local slots; declared locals are initialized to zero.
Function calls recursively create frames with a depth limit, while structured
branches adjust the current program counter and control depth.

i32 addition, subtraction, multiplication, and shifts use unsigned intermediate
values so overflow follows WebAssembly's two's-complement wrapping rules without
triggering C++ signed-overflow undefined behavior. Signed division traps on zero
and on `INT32_MIN / -1`. Loads and stores check a widened address before touching
linear memory.

Linear memory is a byte vector in 64 KiB pages. Active data segments are applied
during instantiation. `memory.grow` returns the old page count or `-1` when the
declared maximum would be exceeded.

The current control implementation accepts empty block signatures. Supporting
typed block results requires preserving branch values while unwinding operand
stack entries; that is the next step toward modern compiler output.

