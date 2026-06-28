#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string_view>
#include <utility>

namespace blola {

template <class Write> struct config {
  [[no_unique_address]] Write _write;

  void write(auto... datas) noexcept {
    std::invoke(_write, std::forward<decltype(datas)>(datas)...);
  }
};

template <class T> struct Sizeof;

template <class T> struct Sizeof {
  T val;
  constexpr void writeTo(std::byte *buffer) const noexcept {
    std::memmove(buffer, &val, size());
  }
  constexpr std::size_t size() const noexcept { return sizeof(val); }
};

template <> struct Sizeof<std::string_view> {
  std::string_view val;
  constexpr void writeTo(std::byte *buffer) const noexcept {
    auto sz = static_cast<std::uint8_t>(val.size());
    std::memmove(buffer, &sz, sizeof(sz));
    std::memmove(buffer + sizeof(sz), val.data(), val.size());
  }
  constexpr std::size_t size() const noexcept { return 1 + val.size(); }
};

template <std::invocable<const std::byte *, std::size_t> Write>
struct double_buffered {
  [[no_unique_address]] Write _write;

  void operator()(auto... datas) noexcept {
    auto totalSize = (Sizeof{datas}.size() + ...);
    std::byte buffer[totalSize];
    std::size_t offset = 0;

    ((Sizeof{datas}.writeTo(buffer + offset), offset += Sizeof{datas}.size()),
     ...);

    std::invoke(_write, static_cast<std::byte *>(buffer), totalSize);
  }
};

} // namespace blola
