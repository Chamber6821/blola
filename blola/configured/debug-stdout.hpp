#pragma once

#include "blola/config.hpp"

#include <ios>
#include <iostream>

#define BLOLA_CONFIG_GLOBAL_VARIABLE_NAME blola__config__

#include "../blola.hpp"

inline auto BLOLA_CONFIG_GLOBAL_VARIABLE_NAME =
    blola::config{[](auto... datas) {
      std::cout << "--- Write ---" << std::endl;
      ((std::cout << std::hex << datas << std::endl), ...);
      std::cout.flush();
    }};
