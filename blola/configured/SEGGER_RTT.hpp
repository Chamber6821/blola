#pragma once

#include <array>
#include <cstring>
#include <iostream>

#include <RTT/SEGGER_RTT.h>

#define BLOLA_CONFIG_GLOBAL_VARIABLE_NAME blola__config__

#ifndef BLOLA_RTT_BUFFER_INDEX
#define BLOLA_RTT_BUFFER_INDEX 0
#endif

#include "../blola.hpp"

struct BLOLA_CONFIG_GLOBAL_VARIABLE_NAME {
  static void write(auto... datas) noexcept {
    constexpr auto totalSize = (sizeof(datas) + ...);
    std::array<std::byte, totalSize> buffer;
    std::size_t offset = 0;

    ((std::memmove(buffer.data() + offset, &datas, sizeof(datas)),
      offset += sizeof(datas)),
     ...);

    SEGGER_RTT_Write(BLOLA_RTT_BUFFER_INDEX, buffer.data(), buffer.size());
  }
};
