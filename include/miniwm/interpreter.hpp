#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "miniwm/module.hpp"
#include "miniwm/value.hpp"

namespace miniwm {

class Interpreter {
 public:
  explicit Interpreter(Module module);
  std::optional<Value> invoke(const std::string& export_name, const std::vector<Value>& arguments);
  std::optional<Value> invoke(std::uint32_t function_index, const std::vector<Value>& arguments);

  const std::vector<std::uint8_t>& memory() const { return memory_; }

 private:
  std::optional<Value> execute(std::uint32_t function_index, const std::vector<Value>& arguments);
  std::uint32_t load_u32(std::uint32_t address) const;
  void store_u32(std::uint32_t address, std::uint32_t value);

  Module module_;
  std::vector<std::uint8_t> memory_;
  std::size_t call_depth_ = 0;
};

}  // namespace miniwm

