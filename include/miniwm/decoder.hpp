#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <limits>

#include "miniwm/error.hpp"

namespace miniwm {

class Decoder {
 public:
  explicit Decoder(const std::vector<std::uint8_t>& bytes) : bytes_(bytes) {}
  Decoder(const std::vector<std::uint8_t>& bytes, std::size_t begin, std::size_t end)
      : bytes_(bytes), position_(begin), end_(end) {}

  std::uint8_t byte();
  std::uint32_t u32();
  std::int32_t s32();
  std::int64_t s64();
  std::string name();
  std::vector<std::uint8_t> bytes(std::size_t count);
  Decoder slice(std::size_t count);

  bool empty() const { return position_ >= limit(); }
  std::size_t position() const { return position_; }
  std::size_t remaining() const { return limit() - position_; }

 private:
  std::size_t limit() const {
    return end_ == std::numeric_limits<std::size_t>::max() ? bytes_.size() : end_;
  }
  const std::vector<std::uint8_t>& bytes_;
  std::size_t position_ = 0;
  std::size_t end_ = std::numeric_limits<std::size_t>::max();
};

}  // namespace miniwm
