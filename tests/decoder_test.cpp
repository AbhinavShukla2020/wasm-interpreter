#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "miniwm/decoder.hpp"

TEST(Decoder, ReadsUnsignedLeb128) {
  const std::vector<std::uint8_t> bytes{0xe5, 0x8e, 0x26};
  miniwm::Decoder decoder(bytes);
  EXPECT_EQ(decoder.u32(), 624485u);
  EXPECT_TRUE(decoder.empty());
}

TEST(Decoder, ReadsSignedLeb128) {
  const std::vector<std::uint8_t> bytes{0x9b, 0xf1, 0x59};
  miniwm::Decoder decoder(bytes);
  EXPECT_EQ(decoder.s32(), -624485);
}

TEST(Decoder, RejectsTruncatedInput) {
  const std::vector<std::uint8_t> bytes{0x80};
  miniwm::Decoder decoder(bytes);
  EXPECT_THROW(decoder.u32(), miniwm::DecodeError);
}

