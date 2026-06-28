#pragma once

#include "blola/config.hpp"

#include "SEGGER_RTT.h"

#define BLOLA_CONFIG_GLOBAL_VARIABLE_NAME blola__config__

#ifndef BLOLA_RTT_BUFFER_INDEX
#define BLOLA_RTT_BUFFER_INDEX 0
#endif

#include "../blola.hpp"

inline auto BLOLA_CONFIG_GLOBAL_VARIABLE_NAME =
    blola::config{blola::double_buffered{[](auto buffer, auto len) {
      SEGGER_RTT_Write(BLOLA_RTT_BUFFER_INDEX, buffer, len);
    }}};
