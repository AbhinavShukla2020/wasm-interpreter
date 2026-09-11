#pragma once

#include <cstdint>
#include <variant>

namespace miniwm {

enum class ValueType : std::uint8_t { I32 = 0x7f, I64 = 0x7e };

class Value {
 public:
  static Value i32(std::int32_t value) { return Value(value); }
  static Value i64(std::int64_t value) { return Value(value); }

  ValueType type() const {
    return std::holds_alternative<std::int32_t>(storage_) ? ValueType::I32 : ValueType::I64;
  }
  std::int32_t as_i32() const { return std::get<std::int32_t>(storage_); }
  std::int64_t as_i64() const { return std::get<std::int64_t>(storage_); }

 private:
  explicit Value(std::int32_t value) : storage_(value) {}
  explicit Value(std::int64_t value) : storage_(value) {}
  std::variant<std::int32_t, std::int64_t> storage_;
};

}  // namespace miniwm

