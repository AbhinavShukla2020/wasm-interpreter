#pragma once

#include <stdexcept>
#include <string>

namespace miniwm {

class DecodeError : public std::runtime_error {
 public:
  explicit DecodeError(const std::string& message) : std::runtime_error(message) {}
};

class Trap : public std::runtime_error {
 public:
  explicit Trap(const std::string& message) : std::runtime_error(message) {}
};

}  // namespace miniwm

