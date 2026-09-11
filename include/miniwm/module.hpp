#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "miniwm/value.hpp"

namespace miniwm {

enum class Op : std::uint8_t {
  Unreachable = 0x00,
  Block = 0x02,
  Loop = 0x03,
  If = 0x04,
  Else = 0x05,
  End = 0x0b,
  Br = 0x0c,
  BrIf = 0x0d,
  Return = 0x0f,
  Call = 0x10,
  Drop = 0x1a,
  Select = 0x1b,
  LocalGet = 0x20,
  LocalSet = 0x21,
  LocalTee = 0x22,
  I32Load = 0x28,
  I32Store = 0x36,
  MemorySize = 0x3f,
  MemoryGrow = 0x40,
  I32Const = 0x41,
  I64Const = 0x42,
  I32Eqz = 0x45,
  I32Eq = 0x46,
  I32Ne = 0x47,
  I32LtS = 0x48,
  I32GtS = 0x4a,
  I32LeS = 0x4c,
  I32GeS = 0x4e,
  I32Add = 0x6a,
  I32Sub = 0x6b,
  I32Mul = 0x6c,
  I32DivS = 0x6d,
  I32RemS = 0x6f,
  I32And = 0x71,
  I32Or = 0x72,
  I32Xor = 0x73,
  I32Shl = 0x74,
  I32ShrS = 0x75,
};

struct FunctionType {
  std::vector<ValueType> parameters;
  std::vector<ValueType> results;
};

struct Instruction {
  Op op;
  std::int64_t immediate = 0;
  std::uint32_t offset = 0;
  std::size_t matching_end = 0;
  std::optional<std::size_t> matching_else;
};

struct Function {
  std::uint32_t type_index = 0;
  std::vector<ValueType> locals;
  std::vector<Instruction> code;
};

struct MemoryType {
  std::uint32_t minimum_pages = 0;
  std::optional<std::uint32_t> maximum_pages;
};

struct DataSegment {
  std::uint32_t offset = 0;
  std::vector<std::uint8_t> bytes;
};

class Module {
 public:
  static Module parse(const std::vector<std::uint8_t>& bytes);

  const Function& function(std::uint32_t index) const;
  const FunctionType& function_type(std::uint32_t function_index) const;
  std::uint32_t exported_function(const std::string& name) const;

  std::vector<FunctionType> types;
  std::vector<Function> functions;
  std::optional<MemoryType> memory;
  std::vector<DataSegment> data;
  std::unordered_map<std::string, std::uint32_t> function_exports;
};

}  // namespace miniwm

