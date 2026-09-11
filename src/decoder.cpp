#include "miniwm/decoder.hpp"

#include <limits>

namespace miniwm {

std::uint8_t Decoder::byte() {
  if (empty()) throw DecodeError("unexpected end of input");
  return bytes_[position_++];
}

std::uint32_t Decoder::u32() {
  std::uint32_t result = 0;
  for (unsigned shift = 0; shift < 35; shift += 7) {
    const auto current = byte();
    if (shift == 28 && (current & 0xf0u) != 0) throw DecodeError("u32 LEB128 overflow");
    result |= static_cast<std::uint32_t>(current & 0x7fu) << shift;
    if ((current & 0x80u) == 0) return result;
  }
  throw DecodeError("invalid u32 LEB128");
}

std::int32_t Decoder::s32() {
  std::uint32_t result = 0;
  unsigned shift = 0;
  while (true) {
    const auto current = byte();
    const auto payload = static_cast<std::uint32_t>(current & 0x7f);
    if (shift == 28 && payload > 0x07u && payload < 0x78u) {
      throw DecodeError("s32 LEB128 overflow");
    }
    result |= payload << shift;
    shift += 7;
    if ((current & 0x80u) == 0) {
      if (shift < 32 && (current & 0x40u)) result |= (~std::uint32_t{0}) << shift;
      return static_cast<std::int32_t>(result);
    }
    if (shift >= 35) throw DecodeError("s32 LEB128 overflow");
  }
}

std::int64_t Decoder::s64() {
  std::uint64_t result = 0;
  unsigned shift = 0;
  while (true) {
    const auto current = byte();
    const auto payload = static_cast<std::uint64_t>(current & 0x7f);
    if (shift == 63 && payload != 0 && payload != 0x7f) {
      throw DecodeError("s64 LEB128 overflow");
    }
    result |= payload << shift;
    shift += 7;
    if ((current & 0x80u) == 0) {
      if (shift < 64 && (current & 0x40u)) result |= (~std::uint64_t{0}) << shift;
      return static_cast<std::int64_t>(result);
    }
    if (shift >= 70) throw DecodeError("s64 LEB128 overflow");
  }
}

std::string Decoder::name() {
  const auto count = u32();
  const auto raw = bytes(count);
  return std::string(raw.begin(), raw.end());
}

std::vector<std::uint8_t> Decoder::bytes(std::size_t count) {
  if (count > remaining()) throw DecodeError("byte vector exceeds section boundary");
  std::vector<std::uint8_t> result(bytes_.begin() + static_cast<std::ptrdiff_t>(position_),
                                   bytes_.begin() + static_cast<std::ptrdiff_t>(position_ + count));
  position_ += count;
  return result;
}

Decoder Decoder::slice(std::size_t count) {
  if (count > remaining()) throw DecodeError("section exceeds input boundary");
  const auto begin = position_;
  position_ += count;
  return Decoder(bytes_, begin, begin + count);
}

}  // namespace miniwm
