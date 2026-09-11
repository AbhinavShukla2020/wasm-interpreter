#include "miniwm/interpreter.hpp"

#include <algorithm>
#include <limits>

#include "miniwm/error.hpp"

namespace miniwm {
namespace {

constexpr std::size_t kPageSize = 65'536;

Value pop(std::vector<Value>& stack) {
  if (stack.empty()) throw Trap("operand stack underflow");
  auto value = stack.back();
  stack.pop_back();
  return value;
}

std::int32_t pop_i32(std::vector<Value>& stack) {
  try {
    return pop(stack).as_i32();
  } catch (const std::bad_variant_access&) {
    throw Trap("expected i32 on operand stack");
  }
}

struct Control {
  Op kind;
  std::size_t opening;
  std::size_t end;
  std::size_t stack_height;
};

}  // namespace

Interpreter::Interpreter(Module module) : module_(std::move(module)) {
  if (module_.memory) {
    memory_.resize(static_cast<std::size_t>(module_.memory->minimum_pages) * kPageSize);
  }
  for (const auto& segment : module_.data) {
    const auto end = static_cast<std::size_t>(segment.offset) + segment.bytes.size();
    if (end > memory_.size()) throw Trap("data segment is outside memory");
    std::copy(segment.bytes.begin(), segment.bytes.end(), memory_.begin() + segment.offset);
  }
}

std::optional<Value> Interpreter::invoke(const std::string& name,
                                         const std::vector<Value>& arguments) {
  return invoke(module_.exported_function(name), arguments);
}

std::optional<Value> Interpreter::invoke(std::uint32_t index,
                                         const std::vector<Value>& arguments) {
  call_depth_ = 0;
  return execute(index, arguments);
}

std::optional<Value> Interpreter::execute(std::uint32_t function_index,
                                          const std::vector<Value>& arguments) {
  if (++call_depth_ > 1'000) {
    --call_depth_;
    throw Trap("call stack exhausted");
  }
  struct DepthGuard {
    std::size_t& depth;
    ~DepthGuard() { --depth; }
  } guard{call_depth_};

  const auto& function = module_.function(function_index);
  const auto& type = module_.function_type(function_index);
  if (arguments.size() != type.parameters.size()) throw Trap("argument count mismatch");
  std::vector<Value> locals = arguments;
  for (const auto local_type : function.locals) {
    locals.push_back(local_type == ValueType::I32 ? Value::i32(0) : Value::i64(0));
  }
  for (std::size_t index = 0; index < arguments.size(); ++index) {
    if (arguments[index].type() != type.parameters[index]) throw Trap("argument type mismatch");
  }

  std::vector<Value> stack;
  std::vector<Control> controls;
  std::size_t pc = 0;
  while (pc < function.code.size()) {
    const auto& instruction = function.code[pc];
    switch (instruction.op) {
      case Op::Unreachable:
        throw Trap("unreachable executed");
      case Op::Block:
      case Op::Loop:
        controls.push_back({instruction.op, pc, instruction.matching_end, stack.size()});
        ++pc;
        break;
      case Op::If: {
        const bool condition = pop_i32(stack) != 0;
        if (condition) {
          controls.push_back({Op::If, pc, instruction.matching_end, stack.size()});
          ++pc;
        } else if (instruction.matching_else) {
          controls.push_back({Op::If, pc, instruction.matching_end, stack.size()});
          pc = *instruction.matching_else + 1;
        } else {
          pc = instruction.matching_end + 1;
        }
        break;
      }
      case Op::Else:
        if (controls.empty() || controls.back().kind != Op::If) throw Trap("unexpected else");
        pc = controls.back().end + 1;
        controls.pop_back();
        break;
      case Op::End:
        if (controls.empty()) {
          pc = function.code.size();
        } else {
          controls.pop_back();
          ++pc;
        }
        break;
      case Op::Br:
      case Op::BrIf: {
        const bool taken = instruction.op == Op::Br || pop_i32(stack) != 0;
        if (!taken) {
          ++pc;
          break;
        }
        const auto depth = static_cast<std::size_t>(instruction.immediate);
        if (depth >= controls.size()) throw Trap("branch depth out of bounds");
        const auto target_index = controls.size() - depth - 1;
        const auto target = controls[target_index];
        if (target.stack_height > stack.size()) throw Trap("invalid control stack height");
        stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(target.stack_height), stack.end());
        if (target.kind == Op::Loop) {
          controls.resize(target_index + 1);
          pc = target.opening + 1;
        } else {
          controls.resize(target_index);
          pc = target.end + 1;
        }
        break;
      }
      case Op::Return:
        pc = function.code.size();
        break;
      case Op::Call: {
        const auto target = static_cast<std::uint32_t>(instruction.immediate);
        const auto& target_type = module_.function_type(target);
        std::vector<Value> call_arguments(target_type.parameters.size(), Value::i32(0));
        for (std::size_t index = target_type.parameters.size(); index > 0; --index) {
          call_arguments[index - 1] = pop(stack);
        }
        if (const auto result = execute(target, call_arguments)) stack.push_back(*result);
        ++pc;
        break;
      }
      case Op::Drop:
        static_cast<void>(pop(stack));
        ++pc;
        break;
      case Op::Select: {
        const auto condition = pop_i32(stack);
        const auto second = pop(stack);
        const auto first = pop(stack);
        if (first.type() != second.type()) throw Trap("select operands have different types");
        stack.push_back(condition ? first : second);
        ++pc;
        break;
      }
      case Op::LocalGet: {
        const auto index = static_cast<std::size_t>(instruction.immediate);
        if (index >= locals.size()) throw Trap("local index out of bounds");
        stack.push_back(locals[index]);
        ++pc;
        break;
      }
      case Op::LocalSet:
      case Op::LocalTee: {
        const auto index = static_cast<std::size_t>(instruction.immediate);
        if (index >= locals.size()) throw Trap("local index out of bounds");
        const auto value = pop(stack);
        locals[index] = value;
        if (instruction.op == Op::LocalTee) stack.push_back(value);
        ++pc;
        break;
      }
      case Op::I32Const:
        stack.push_back(Value::i32(static_cast<std::int32_t>(instruction.immediate)));
        ++pc;
        break;
      case Op::I64Const:
        stack.push_back(Value::i64(instruction.immediate));
        ++pc;
        break;
      case Op::I32Eqz: {
        stack.push_back(Value::i32(pop_i32(stack) == 0));
        ++pc;
        break;
      }
      case Op::I32Eq:
      case Op::I32Ne:
      case Op::I32LtS:
      case Op::I32GtS:
      case Op::I32LeS:
      case Op::I32GeS:
      case Op::I32Add:
      case Op::I32Sub:
      case Op::I32Mul:
      case Op::I32DivS:
      case Op::I32RemS:
      case Op::I32And:
      case Op::I32Or:
      case Op::I32Xor:
      case Op::I32Shl:
      case Op::I32ShrS: {
        const auto right = pop_i32(stack);
        const auto left = pop_i32(stack);
        std::int32_t result = 0;
        switch (instruction.op) {
          case Op::I32Eq: result = left == right; break;
          case Op::I32Ne: result = left != right; break;
          case Op::I32LtS: result = left < right; break;
          case Op::I32GtS: result = left > right; break;
          case Op::I32LeS: result = left <= right; break;
          case Op::I32GeS: result = left >= right; break;
          case Op::I32Add: result = static_cast<std::uint32_t>(left) + static_cast<std::uint32_t>(right); break;
          case Op::I32Sub: result = static_cast<std::uint32_t>(left) - static_cast<std::uint32_t>(right); break;
          case Op::I32Mul: result = static_cast<std::uint32_t>(left) * static_cast<std::uint32_t>(right); break;
          case Op::I32DivS:
            if (right == 0) throw Trap("integer divide by zero");
            if (left == std::numeric_limits<std::int32_t>::min() && right == -1) throw Trap("integer overflow");
            result = left / right;
            break;
          case Op::I32RemS:
            if (right == 0) throw Trap("integer divide by zero");
            result = left == std::numeric_limits<std::int32_t>::min() && right == -1 ? 0 : left % right;
            break;
          case Op::I32And: result = left & right; break;
          case Op::I32Or: result = left | right; break;
          case Op::I32Xor: result = left ^ right; break;
          case Op::I32Shl: result = static_cast<std::uint32_t>(left) << (right & 31); break;
          case Op::I32ShrS: {
            const auto shift = static_cast<unsigned>(right) & 31u;
            auto shifted = static_cast<std::uint32_t>(left) >> shift;
            if (left < 0 && shift != 0) shifted |= (~std::uint32_t{0}) << (32u - shift);
            result = static_cast<std::int32_t>(shifted);
            break;
          }
          default: break;
        }
        stack.push_back(Value::i32(result));
        ++pc;
        break;
      }
      case Op::I32Load: {
        const auto address = static_cast<std::uint32_t>(pop_i32(stack)) + instruction.offset;
        stack.push_back(Value::i32(static_cast<std::int32_t>(load_u32(address))));
        ++pc;
        break;
      }
      case Op::I32Store: {
        const auto value = static_cast<std::uint32_t>(pop_i32(stack));
        const auto address = static_cast<std::uint32_t>(pop_i32(stack)) + instruction.offset;
        store_u32(address, value);
        ++pc;
        break;
      }
      case Op::MemorySize:
        stack.push_back(Value::i32(static_cast<std::int32_t>(memory_.size() / kPageSize)));
        ++pc;
        break;
      case Op::MemoryGrow: {
        const auto delta = static_cast<std::uint32_t>(pop_i32(stack));
        const auto old_pages = static_cast<std::uint32_t>(memory_.size() / kPageSize);
        const auto new_pages = static_cast<std::uint64_t>(old_pages) + delta;
        if (!module_.memory || new_pages > 65'536 ||
            (module_.memory->maximum_pages && new_pages > *module_.memory->maximum_pages)) {
          stack.push_back(Value::i32(-1));
        } else {
          memory_.resize(static_cast<std::size_t>(new_pages) * kPageSize);
          stack.push_back(Value::i32(static_cast<std::int32_t>(old_pages)));
        }
        ++pc;
        break;
      }
    }
  }

  if (type.results.empty()) return std::nullopt;
  if (stack.empty() || stack.back().type() != type.results[0]) throw Trap("function result mismatch");
  return stack.back();
}

std::uint32_t Interpreter::load_u32(std::uint32_t address) const {
  if (static_cast<std::uint64_t>(address) + 4 > memory_.size()) throw Trap("out-of-bounds memory load");
  return static_cast<std::uint32_t>(memory_[address]) |
         (static_cast<std::uint32_t>(memory_[address + 1]) << 8u) |
         (static_cast<std::uint32_t>(memory_[address + 2]) << 16u) |
         (static_cast<std::uint32_t>(memory_[address + 3]) << 24u);
}

void Interpreter::store_u32(std::uint32_t address, std::uint32_t value) {
  if (static_cast<std::uint64_t>(address) + 4 > memory_.size()) throw Trap("out-of-bounds memory store");
  memory_[address] = static_cast<std::uint8_t>(value);
  memory_[address + 1] = static_cast<std::uint8_t>(value >> 8u);
  memory_[address + 2] = static_cast<std::uint8_t>(value >> 16u);
  memory_[address + 3] = static_cast<std::uint8_t>(value >> 24u);
}

}  // namespace miniwm
