#include "miniwm/module.hpp"

#include <array>
#include <utility>

#include "miniwm/decoder.hpp"
#include "miniwm/error.hpp"

namespace miniwm {
namespace {

ValueType read_value_type(Decoder& decoder) {
  const auto value = decoder.byte();
  if (value == static_cast<std::uint8_t>(ValueType::I32)) return ValueType::I32;
  if (value == static_cast<std::uint8_t>(ValueType::I64)) return ValueType::I64;
  throw DecodeError("only i32 and i64 value types are supported");
}

std::vector<ValueType> read_types(Decoder& decoder) {
  const auto count = decoder.u32();
  std::vector<ValueType> result;
  result.reserve(count);
  for (std::uint32_t index = 0; index < count; ++index) result.push_back(read_value_type(decoder));
  return result;
}

Instruction read_instruction(Decoder& decoder) {
  Instruction instruction{static_cast<Op>(decoder.byte()), 0, 0, 0, std::nullopt};
  switch (instruction.op) {
    case Op::Block:
    case Op::Loop:
    case Op::If:
      if (decoder.byte() != 0x40) throw DecodeError("only empty block signatures are supported");
      break;
    case Op::Br:
    case Op::BrIf:
    case Op::Call:
    case Op::LocalGet:
    case Op::LocalSet:
    case Op::LocalTee:
      instruction.immediate = decoder.u32();
      break;
    case Op::I32Const:
      instruction.immediate = decoder.s32();
      break;
    case Op::I64Const:
      instruction.immediate = decoder.s64();
      break;
    case Op::I32Load:
    case Op::I32Store:
      static_cast<void>(decoder.u32());  // alignment hint
      instruction.offset = decoder.u32();
      break;
    case Op::MemorySize:
    case Op::MemoryGrow:
      if (decoder.byte() != 0) throw DecodeError("invalid memory instruction reserved byte");
      break;
    case Op::Unreachable:
    case Op::Else:
    case Op::End:
    case Op::Return:
    case Op::Drop:
    case Op::Select:
    case Op::I32Eqz:
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
    case Op::I32ShrS:
      break;
    default:
      throw DecodeError("unsupported opcode");
  }
  return instruction;
}

void link_control_flow(std::vector<Instruction>& code) {
  std::vector<std::size_t> open;
  for (std::size_t index = 0; index < code.size(); ++index) {
    const auto op = code[index].op;
    if (op == Op::Block || op == Op::Loop || op == Op::If) {
      open.push_back(index);
    } else if (op == Op::Else) {
      if (open.empty() || code[open.back()].op != Op::If) throw DecodeError("else without matching if");
      code[open.back()].matching_else = index;
      open.push_back(index);
    } else if (op == Op::End && !open.empty()) {
      const auto opening = open.back();
      open.pop_back();
      code[opening].matching_end = index;
      if (code[opening].op == Op::Else) {
        if (open.empty() || code[open.back()].op != Op::If) throw DecodeError("malformed if/else");
        code[open.back()].matching_end = index;
        open.pop_back();
      }
    }
  }
  if (!open.empty() || code.empty() || code.back().op != Op::End) {
    throw DecodeError("function has unbalanced control flow");
  }
}

}  // namespace

Module Module::parse(const std::vector<std::uint8_t>& bytes) {
  Decoder decoder(bytes);
  const std::array<std::uint8_t, 8> header{0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};
  for (const auto expected : header) {
    if (decoder.byte() != expected) throw DecodeError("invalid WebAssembly magic or version");
  }

  Module module;
  std::vector<std::uint32_t> function_types;
  std::uint8_t last_section = 0;
  while (!decoder.empty()) {
    const auto section_id = decoder.byte();
    auto section = decoder.slice(decoder.u32());
    if (section_id != 0 && section_id <= last_section) throw DecodeError("sections are out of order");
    if (section_id != 0) last_section = section_id;
    switch (section_id) {
      case 0:
        static_cast<void>(section.bytes(section.remaining()));
        break;
      case 1: {
        const auto count = section.u32();
        for (std::uint32_t index = 0; index < count; ++index) {
          if (section.byte() != 0x60) throw DecodeError("expected function type");
          auto parameters = read_types(section);
          auto results = read_types(section);
          if (results.size() > 1) throw DecodeError("multiple results are not supported");
          module.types.push_back({std::move(parameters), std::move(results)});
        }
        break;
      }
      case 3: {
        const auto count = section.u32();
        for (std::uint32_t index = 0; index < count; ++index) function_types.push_back(section.u32());
        break;
      }
      case 5: {
        if (section.u32() != 1) throw DecodeError("only one memory is supported");
        const auto flags = section.u32();
        MemoryType memory{section.u32(), std::nullopt};
        if (flags == 1) memory.maximum_pages = section.u32();
        else if (flags != 0) throw DecodeError("unsupported memory flags");
        module.memory = memory;
        break;
      }
      case 7: {
        const auto count = section.u32();
        for (std::uint32_t index = 0; index < count; ++index) {
          auto name = section.name();
          const auto kind = section.byte();
          const auto target = section.u32();
          if (kind == 0) module.function_exports.emplace(std::move(name), target);
        }
        break;
      }
      case 10: {
        const auto count = section.u32();
        if (count != function_types.size()) throw DecodeError("function and code counts differ");
        for (std::uint32_t function_index = 0; function_index < count; ++function_index) {
          auto body = section.slice(section.u32());
          Function function;
          function.type_index = function_types[function_index];
          const auto local_groups = body.u32();
          for (std::uint32_t group = 0; group < local_groups; ++group) {
            const auto amount = body.u32();
            const auto type = read_value_type(body);
            function.locals.insert(function.locals.end(), amount, type);
          }
          while (!body.empty()) function.code.push_back(read_instruction(body));
          link_control_flow(function.code);
          module.functions.push_back(std::move(function));
        }
        break;
      }
      case 11: {
        const auto count = section.u32();
        for (std::uint32_t index = 0; index < count; ++index) {
          if (section.u32() != 0 || section.byte() != static_cast<std::uint8_t>(Op::I32Const)) {
            throw DecodeError("only active data segments are supported");
          }
          const auto offset = section.s32();
          if (offset < 0 || section.byte() != static_cast<std::uint8_t>(Op::End)) {
            throw DecodeError("invalid data offset expression");
          }
          module.data.push_back({static_cast<std::uint32_t>(offset), section.bytes(section.u32())});
        }
        break;
      }
      default:
        throw DecodeError("unsupported module section");
    }
    if (!section.empty()) throw DecodeError("section was not fully consumed");
  }
  for (const auto& function : module.functions) {
    if (function.type_index >= module.types.size()) throw DecodeError("function type index out of bounds");
  }
  return module;
}

const Function& Module::function(std::uint32_t index) const {
  if (index >= functions.size()) throw DecodeError("function index out of bounds");
  return functions[index];
}

const FunctionType& Module::function_type(std::uint32_t function_index) const {
  return types.at(function(function_index).type_index);
}

std::uint32_t Module::exported_function(const std::string& name) const {
  const auto found = function_exports.find(name);
  if (found == function_exports.end()) throw DecodeError("function export not found: " + name);
  return found->second;
}

}  // namespace miniwm
