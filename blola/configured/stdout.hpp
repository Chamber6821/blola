#pragma once

#include <array>
#include <cstring>
#include <iostream>

#define BLOLA_CONFIG_GLOBAL_VARIABLE_NAME blola__config__

#include "../blola.hpp"

struct BLOLA_CONFIG_GLOBAL_VARIABLE_NAME {
  static void write(auto... datas) noexcept {
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
};
