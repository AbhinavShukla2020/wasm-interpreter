#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "miniwm/interpreter.hpp"
#include "miniwm/module.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "usage: miniwm module.wasm export_name [i32 arguments...]\n";
    return 2;
  }
  try {
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) throw std::runtime_error("could not open module");
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input), {}};
    miniwm::Interpreter interpreter(miniwm::Module::parse(bytes));
    std::vector<miniwm::Value> arguments;
    for (int index = 3; index < argc; ++index) {
      arguments.push_back(miniwm::Value::i32(std::stoi(argv[index])));
    }
    const auto result = interpreter.invoke(argv[2], arguments);
    if (result) {
      if (result->type() == miniwm::ValueType::I32) std::cout << result->as_i32() << '\n';
      else std::cout << result->as_i64() << '\n';
    }
  } catch (const std::exception& error) {
    std::cerr << "miniwm: " << error.what() << '\n';
    return 1;
  }
}

