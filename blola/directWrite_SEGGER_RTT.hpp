#pragma once

#include "blola.hpp"

#include <array>
#include <cstring>

#include <RTT/SEGGER_RTT.h>

#ifndef BLOLA_RTT_BUFFER_INDEX
#define BLOLA_RTT_BUFFER_INDEX 0
#endif

namespace blola {
void directWrite(adl_tag, auto... datas) noexcept {
  constexpr auto totalSize = (sizeof(datas) + ...);
  std::array<std::byte, totalSize> buffer;
  std::size_t offset = 0;

  ((std::memmove(buffer.data() + offset, &datas, sizeof(datas)),
    offset += sizeof(datas)),
   ...);

  SEGGER_RTT_Write(BLOLA_RTT_BUFFER_INDEX, buffer.data(), buffer.size());
}
} // namespace blola
