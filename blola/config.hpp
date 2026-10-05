#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace blola {

template <class Write> struct config {
  [[no_unique_address]] Write _write;

  void write(auto... datas) noexcept {
    std::invoke(_write, std::forward<decltype(datas)>(datas)...);
  }
};

namespace detail {

template <class T>
inline constexpr bool is_string_view_v =
    std::is_same_v<std::remove_cvref_t<T>, std::string_view>;

template <class T> struct ScalarPart {
  T val;
  static constexpr std::size_t size() noexcept { return sizeof(T); }
  constexpr void writeTo(std::byte *buffer) const noexcept {
    std::memmove(buffer, &val, size());
  }
};

template <> struct ScalarPart<std::string_view> {
  std::string_view val;
  static constexpr std::size_t size() noexcept { return 1; }
  constexpr void writeTo(std::byte *buffer) const noexcept {
    auto sz = static_cast<std::uint8_t>(val.size());
    std::memmove(buffer, &sz, 1);
  }
};

template <std::size_t Begin, std::size_t Count, class Seq> struct offset_seq;
template <std::size_t Begin, std::size_t Count, std::size_t... Ks>
struct offset_seq<Begin, Count, std::index_sequence<Ks...>> {
  using type = std::index_sequence<(Begin + Ks)...>;
};
template <std::size_t Begin, std::size_t End>
using range_seq_t =
    typename offset_seq<Begin, End - Begin,
                        std::make_index_sequence<End - Begin>>::type;

template <class Write, class Tuple, std::size_t... Is>
constexpr void emit_scalar_segment(Write &write, Tuple &&t,
                                   std::index_sequence<Is...>) {
  using TupleT = std::remove_reference_t<Tuple>;
  constexpr std::size_t segSize =
      (ScalarPart<std::remove_cvref_t<std::tuple_element_t<Is, TupleT>>>::size() + ...);
  std::array<std::byte, segSize> buffer{};
  std::size_t offset = 0;
  (
      [&] {
        using Part =
            ScalarPart<std::remove_cvref_t<std::tuple_element_t<Is, TupleT>>>;
        Part part{std::get<Is>(std::forward<Tuple>(t))};
        part.writeTo(buffer.data() + offset);
        offset += Part::size();
      }(),
      ...);
  std::invoke(write, buffer.data(), segSize);
}

template <class Write, class Tuple, std::size_t I>
constexpr void emit_string(Write &write, Tuple &&t) {
  std::string_view sv = std::get<I>(std::forward<Tuple>(t));
  std::invoke(write, reinterpret_cast<const std::byte *>(sv.data()), sv.size());
}

template <class Write, class Tuple, std::size_t Begin, std::size_t End>
constexpr void emit_scalar_range(Write &write, Tuple &&t) {
  if constexpr (End > Begin)
    emit_scalar_segment(write, std::forward<Tuple>(t), range_seq_t<Begin, End>{});
}

template <class Write, class Tuple, std::size_t Begin, std::size_t N,
          std::size_t First, std::size_t... Rest>
constexpr void emit_loop(Write &write, Tuple &&t,
                         std::index_sequence<First, Rest...>) {
  emit_scalar_range<Write, Tuple, Begin, First + 1>(write,
                                                      std::forward<Tuple>(t));
  emit_string<Write, Tuple, First>(write, std::forward<Tuple>(t));
  if constexpr (sizeof...(Rest) > 0)
    emit_loop<Write, Tuple, First + 1, N>(write, std::forward<Tuple>(t),
                                          std::index_sequence<Rest...>{});
  else
    emit_scalar_range<Write, Tuple, First + 1, N>(write,
                                                   std::forward<Tuple>(t));
}

template <class Write, class Tuple, std::size_t Begin, std::size_t N>
constexpr void emit_loop(Write &write, Tuple &&t, std::index_sequence<>) {
  emit_scalar_range<Write, Tuple, Begin, N>(write, std::forward<Tuple>(t));
}

template <class Write, class Tuple, std::size_t N, std::size_t... SvIdx>
constexpr void emit_all(Write &write, Tuple &&t,
                        std::index_sequence<SvIdx...>) {
  emit_loop<Write, Tuple, 0, N>(write, std::forward<Tuple>(t),
                                std::index_sequence<SvIdx...>{});
}

template <class Tuple, std::size_t... Is>
constexpr auto collect_sv_indices(std::index_sequence<Is...>) {
  constexpr std::size_t N = sizeof...(Is);
  constexpr bool isSv[] = {
      is_string_view_v<std::tuple_element_t<Is, std::remove_reference_t<Tuple>>>...};
  constexpr std::size_t count = ((isSv[Is] ? std::size_t{1} : std::size_t{0}) + ... + 0);
  std::array<std::size_t, count> result{};
  std::size_t pos = 0;
  for (std::size_t i = 0; i < N; ++i)
    if (isSv[i])
      result[pos++] = i;
  return result;
}

} // namespace detail

template <std::invocable<const std::byte *, std::size_t> Write>
struct double_buffered {
  [[no_unique_address]] Write _write;

  void operator()(auto... datas) noexcept {
    constexpr std::size_t N = sizeof...(datas);
    if constexpr (N == 0) {
      return;
    } else {
      auto tup = std::forward_as_tuple(datas...);
      using Tuple = decltype(tup);

      constexpr auto svIndices =
          detail::collect_sv_indices<Tuple>(std::make_index_sequence<N>{});

      [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
        detail::emit_all<Write, Tuple, N>(
            _write, std::forward<Tuple>(tup),
            std::index_sequence<svIndices[Ks]...>{});
      }(std::make_index_sequence<svIndices.size()>{});
    }
  }
};

} // namespace blola
