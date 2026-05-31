#pragma once

#include <algorithm>
#include <optional>
#include <string_view>

namespace blola {

template <class... args> struct Pack {};

template <size_t N> struct CtStr {
  char data[N];

  constexpr CtStr(const char (&str)[N]) noexcept { std::copy_n(str, N, data); }

  constexpr CtStr(std::string_view str) noexcept {
    std::copy_n(str.data(), N - 1, data);
    data[N - 1] = 0;
  }

  constexpr std::string_view view() const noexcept { return {data, data + N}; }
};

} // namespace blola

namespace blola::format {

consteval bool isSpecifierTypeChar(char ch) {
  return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z');
}

consteval std::optional<std::size_t> findSpecifier(std::string_view str,
                                                   std::size_t start = 0) {
  for (auto offset = start; offset < str.size();) {
    offset = str.find('%', offset);
    if (offset == str.npos)
      return std::nullopt;
    if (str.substr(offset).starts_with("%%")) {
      offset += 2;
      continue;
    }
    while (offset < str.size() && not isSpecifierTypeChar(str[offset]))
      offset++;
    if (offset < str.size())
      return offset;
  }
  return std::nullopt;
}

consteval std::optional<std::size_t> findNthSpecifier(std::string_view str,
                                                      std::size_t n) {
  std::size_t offset = 0;
  for (std::size_t i = 0; i < n + 1; i++) {
    auto f = findSpecifier(str, offset);
    if (!f)
      return std::nullopt;
    offset = *f;
  }
  return offset;
}

consteval std::size_t countSpecifiers(std::string_view str) {
  std::size_t i = 0;
  while (findNthSpecifier(str, i).has_value())
    i++;
  return i;
}

enum class TypeTag {
  c,
  si,
  i,
  li,
  lli,
  im,
  uc,
  usi,
  ui,
  uli,
  ulli,
  uim,
  f,
  d,
  size,
  ptr
};

template <class T> using tid = std::type_identity<T>;

template <TypeTag Tag> struct tagToType;
template <> struct tagToType<TypeTag::c> : tid<signed char> {};
template <> struct tagToType<TypeTag::si> : tid<short int> {};
template <> struct tagToType<TypeTag::i> : tid<int> {};
template <> struct tagToType<TypeTag::li> : tid<long int> {};
template <> struct tagToType<TypeTag::lli> : tid<long long int> {};
template <> struct tagToType<TypeTag::im> : tid<long long int> {};
template <> struct tagToType<TypeTag::uc> : tid<unsigned char> {};
template <> struct tagToType<TypeTag::usi> : tid<unsigned short int> {};
template <> struct tagToType<TypeTag::ui> : tid<unsigned int> {};
template <> struct tagToType<TypeTag::uli> : tid<unsigned long int> {};
template <> struct tagToType<TypeTag::ulli> : tid<unsigned long long int> {};
template <> struct tagToType<TypeTag::uim> : tid<unsigned long long int> {};
template <> struct tagToType<TypeTag::f> : tid<float> {};
template <> struct tagToType<TypeTag::d> : tid<double> {};
template <> struct tagToType<TypeTag::size> : tid<std::size_t> {};
template <> struct tagToType<TypeTag::ptr> : tid<void *> {};

template <TypeTag Tag> using tagToType_t = typename tagToType<Tag>::type;

consteval bool startsWithAny(std::string_view str, auto... patterns) {
  return (str.starts_with(patterns) || ...);
}

consteval std::optional<TypeTag> matchType(std::string_view specifier) {
  using enum TypeTag;
  if (startsWithAny(specifier, "hhd", "hhi"))
    return c;
  if (startsWithAny(specifier, "hd", "hi"))
    return si;
  if (startsWithAny(specifier, "d", "i"))
    return i;
  if (startsWithAny(specifier, "ld", "li"))
    return li;
  if (startsWithAny(specifier, "lld", "lli"))
    return lli;
  if (startsWithAny(specifier, "jd", "ji"))
    return im;
  if (startsWithAny(specifier, "hhu", "hhx", "hhX", "hho"))
    return uc;
  if (startsWithAny(specifier, "hu", "hx", "hX", "ho"))
    return usi;
  if (startsWithAny(specifier, "u", "x", "X", "o"))
    return ui;
  if (startsWithAny(specifier, "lu", "lx", "lX", "lo"))
    return uli;
  if (startsWithAny(specifier, "llu", "llx", "llX", "llo"))
    return ulli;
  if (startsWithAny(specifier, "ju", "jx", "jX", "jo"))
    return uim;
  if (startsWithAny(specifier, "zu", "zx", "zX", "zo"))
    return size;
  if (startsWithAny(specifier, "f", "e", "E"))
    return f;
  if (startsWithAny(specifier, "lf", "le", "lE"))
    return d;
  if (startsWithAny(specifier, "p"))
    return ptr;
  return std::nullopt;
}

consteval std::optional<std::string_view>
findUnknownSpecifier(std::string_view str) {
  std::size_t index = 0;
  auto pos = findNthSpecifier(str, index);
  while (pos.has_value() && matchType(str.substr(*pos)).has_value()) {
    index++;
    pos = findNthSpecifier(str, index);
  }
  if (not pos.has_value())
    return std::nullopt;
  auto start = str.rfind('%', *pos);
  auto len = str.find_first_not_of("lhjz", *pos) - start + 1;
  return str.substr(start, len);
}

template <class T> struct AutoCast : tid<T> {};
template <class T> struct AutoCast<T *> : tid<void *> {};
template <> struct AutoCast<std::nullptr_t> : tid<void *> {};
template <class T> using AutoCast_t = typename AutoCast<T>::type;

struct valid_types {
private:
  template <auto...> struct always_false : std::false_type {};

  template <auto format, auto unknownSpecifier>
  constexpr static void unknown_specifier() {
    static_assert(always_false<format, unknownSpecifier>::value,
                  "Unknown specifier");
  }

  template <auto format, class... ExpectedTypes, class... RealTypes>
  constexpr static void
  specifier_and_arg_type_mismatch(Pack<ExpectedTypes...> expected,
                                  Pack<RealTypes...> real) {
    static_assert(always_false<format, expected, real>::value,
                  "Specifier and arg type mismatch");
  }

  template <auto format, class... ExpectedTypes, class... RealTypes>
  constexpr static void too_many_specifiers(Pack<ExpectedTypes...> expected,
                                            Pack<RealTypes...> real) {
    static_assert(always_false<format, expected, real>::value,
                  "Too many specifiers");
  }

  template <auto format, class... ExpectedTypes, class... RealTypes>
  constexpr static void too_many_args(Pack<ExpectedTypes...> expected,
                                      Pack<RealTypes...> real) {
    static_assert(always_false<format, expected, real>::value, "Too many args");
  }

  template <CtStr format, std::size_t I>
  constexpr static auto specifierTypeTag = matchType(
      format.view().substr(findNthSpecifier(format.view(), I).value_or(0)));

  template <CtStr format, std::size_t... I>
  using ExpectedPack = Pack<
      tagToType_t<matchType(format.view().substr(
                                findNthSpecifier(format.view(), I).value_or(0)))
                      .value_or(TypeTag::ptr)>...>;

  template <class... Args>
  using RealPack = Pack<AutoCast_t<std::remove_cv_t<Args>>...>;

  constexpr valid_types() = default;

  template <CtStr format, auto... SpecN, class... Args>
  consteval static valid_types validate_impl(std::index_sequence<SpecN...>,
                                             Pack<Args...>) {
    constexpr auto unknownSpecifier = findUnknownSpecifier(format.view());
    if constexpr (unknownSpecifier.has_value()) {
      unknown_specifier<format, CtStr<unknownSpecifier->size() + 1>{
                                    *unknownSpecifier}>();
    } else {
      using expected = ExpectedPack<format, SpecN...>;
      using real = RealPack<Args...>;
      if constexpr (sizeof...(SpecN) > sizeof...(Args)) {
        too_many_specifiers<format>(expected{}, real{});
      } else if constexpr (sizeof...(SpecN) < sizeof...(Args)) {
        too_many_args<format>(expected{}, real{});
      } else if constexpr (not std::same_as<expected, real>) {
        specifier_and_arg_type_mismatch<format>(expected{}, real{});
      }
    }

    return {};
  }

public:
  template <CtStr format, class... Args>
  consteval static valid_types validate() {
    return validate_impl<format>(
        std::make_index_sequence<countSpecifiers(format.view())>{},
        Pack<Args...>{});
  }
};

} // namespace blola::format
