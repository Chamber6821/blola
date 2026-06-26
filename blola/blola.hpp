#pragma once

#include "format.hpp"
#include "platform.hpp"

#include <bit>
#include <climits>
#include <concepts>
#include <cstdint>
#include <string_view>
#include <type_traits>

#ifndef BLOLA_CONFIG_GLOBAL_VARIABLE_NAME
#error                                                                         \
    "You must define BLOLA_CONFIG_GLOBAL_VARIABLE_NAME for your blola config." \
"See blola/configured/stdout.hpp"
#endif

namespace blola {

namespace hash {

namespace function {

template <std::size_t WordSize> struct polynom {
  constexpr static auto operator()(std::unsigned_integral auto hash,
                                   std::unsigned_integral auto val) noexcept
      -> decltype(hash) {
    for (std::size_t i = 0; i < sizeof(val); i += WordSize) {
      hash *= 31u;
      hash += val & ((decltype(hash){1} << (WordSize * CHAR_BIT)) - 1);
      if constexpr (sizeof(val) > WordSize)
        val >>= WordSize * CHAR_BIT;
    }
    return hash;
  }
};

struct _xor {
  constexpr static auto operator()(std::unsigned_integral auto hash,
                                   std::unsigned_integral auto val) noexcept
      -> decltype(hash) {
    constexpr auto WordSize = sizeof(hash);
    for (std::size_t i = 0; i < sizeof(val); i += WordSize) {
      hash ^= val & ((1u << (WordSize * CHAR_BIT)) - 1);
      if constexpr (sizeof(val) > WordSize)
        val >>= WordSize * CHAR_BIT;
    }
    return hash;
  }
};

} // namespace function

template <class TVal, class HashFunc> class Hash {
  TVal sum;

public:
  constexpr Hash(TVal sum = {}) noexcept : sum(sum) {}

  constexpr TVal asUint() const noexcept { return sum; }

  constexpr Hash add(std::unsigned_integral auto val) const noexcept {
    return HashFunc{}(sum, val);
  }

  constexpr Hash add(std::signed_integral auto val) const noexcept {
    return add(static_cast<std::make_unsigned_t<decltype(val)>>(val));
  }

  constexpr Hash add(float val) const noexcept {
    return add(std::bit_cast<std::uint32_t>(val));
  }

  constexpr Hash add(double val) const noexcept {
    return add(std::bit_cast<std::uint64_t>(val));
  }

  constexpr Hash add(std::string_view val) const noexcept {
    auto copy = *this;
    for (std::uint8_t ch : val)
      copy = copy.add(ch);
    return copy;
  }

  constexpr Hash add(auto *val) const noexcept {
    return add(reinterpret_cast<std::intptr_t>(val));
  }

  constexpr Hash add(auto... vals) const noexcept {
    auto copy = *this;
    ((copy = copy.add(vals)), ...);
    return copy;
  }
};

} // namespace hash

using HashVal = std::uint16_t;
using IdHash = hash::Hash<HashVal, hash::function::polynom<1>>;
using BodyHash = hash::Hash<HashVal, hash::function::_xor>;

consteval auto logId(std::string_view filename, std::uint32_t line,
                     std::string_view msg, const char *tailTag) noexcept {
  return IdHash{}.add(line, filename, msg).asUint();
}

template <CtStr format, class Config> void log(auto id, auto... args) {
  [[maybe_unused]] constexpr auto _ =
      format::valid_types::validate<format, decltype(args)...>();
  Config::write(id, BodyHash{}.add(id, args...).asUint(), args...);
}

} // namespace blola

#define blog(MSG, ...)                                                         \
  blola::log<(MSG), BLOLA_CONFIG_GLOBAL_VARIABLE_NAME>(                        \
      ::blola::logId(__FILE__, __LINE__, (MSG), "BLOLA_LOG_ID_END")            \
          __VA_OPT__(, ) __VA_ARGS__)
