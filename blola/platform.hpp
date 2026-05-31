#pragma once

namespace blola {
struct platform;
}

struct blola::platform {
  inline static auto size_short = __SIZEOF_SHORT__;
  inline static auto size_int = __SIZEOF_INT__;
  inline static auto size_long = __SIZEOF_LONG__;
  inline static auto size_long_long = __SIZEOF_LONG_LONG__;
  inline static auto size_size = __SIZEOF_SIZE_T__;
  inline static auto size_ptr = __SIZEOF_POINTER__;
};
