#include <cstdint>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "miniwm/error.hpp"
#include "miniwm/interpreter.hpp"
#include "miniwm/module.hpp"

namespace {

std::vector<std::uint8_t> add_module() {
  return {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
      0x01, 0x07, 0x01, 0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f,
      0x03, 0x02, 0x01, 0x00,
      0x07, 0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00,
      0x0a, 0x09, 0x01, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b,
  };
}

miniwm::Module arithmetic_module(miniwm::Op op) {
  miniwm::Module module;
  module.types.push_back({{miniwm::ValueType::I32, miniwm::ValueType::I32},
                          {miniwm::ValueType::I32}});
  module.functions.push_back({0, {}, {{miniwm::Op::LocalGet, 0},
                                      {miniwm::Op::LocalGet, 1},
                                      {op},
                                      {miniwm::Op::End}}});
  module.function_exports["run"] = 0;
  return module;
}

}  // namespace

TEST(Module, RejectsBadMagic) {
  auto bytes = add_module();
  bytes[0] = 1;
  EXPECT_THROW(miniwm::Module::parse(bytes), miniwm::DecodeError);
}

TEST(Module, ParsesFunctionExport) {
  const auto module = miniwm::Module::parse(add_module());
  EXPECT_EQ(module.exported_function("add"), 0u);
  EXPECT_EQ(module.functions.size(), 1u);
}

TEST(Interpreter, InvokesExportedAdd) {
  miniwm::Interpreter interpreter(miniwm::Module::parse(add_module()));
  const auto result = interpreter.invoke("add", {miniwm::Value::i32(19), miniwm::Value::i32(23)});
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->as_i32(), 42);
}

TEST(Interpreter, WrapsIntegerAddition) {
  miniwm::Interpreter interpreter(arithmetic_module(miniwm::Op::I32Add));
  const auto result = interpreter.invoke(
      "run", {miniwm::Value::i32(std::numeric_limits<std::int32_t>::max()), miniwm::Value::i32(1)});
  EXPECT_EQ(result->as_i32(), std::numeric_limits<std::int32_t>::min());
}

TEST(Interpreter, TrapsIntegerDivisionByZero) {
  miniwm::Interpreter interpreter(arithmetic_module(miniwm::Op::I32DivS));
  EXPECT_THROW(
      interpreter.invoke("run", {miniwm::Value::i32(1), miniwm::Value::i32(0)}),
      miniwm::Trap);
}

TEST(Interpreter, LoadsActiveDataSegment) {
  miniwm::Module module;
  module.memory = miniwm::MemoryType{1, 1};
  module.data.push_back({4, {1, 2, 3, 4}});
  miniwm::Interpreter interpreter(std::move(module));
  EXPECT_EQ(interpreter.memory()[4], 1);
  EXPECT_EQ(interpreter.memory()[7], 4);
}
