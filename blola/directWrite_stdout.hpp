#pragma once

#include "blola.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace blola {
void directWrite(adl_tag, auto... datas) noexcept {
  constexpr auto totalSize = (sizeof(datas) + ...);
  std::array<std::byte, totalSize> buffer;
  std::size_t offset = 0;

  ((std::memmove(buffer.data() + offset, &datas, sizeof(datas)),
    offset += sizeof(datas)),
   ...);

  for (auto byte : buffer) {
    std::cout.put(static_cast<std::uint8_t>(byte));
  }
  std::cout.flush();
}
} // namespace blola
