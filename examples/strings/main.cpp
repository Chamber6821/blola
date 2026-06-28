#include <blola/configured/stdout.hpp>
#include <string_view>

int main() {
  blog("C string: %s", "aboba2");
  blog("std::string_view: %s", std::string_view("view"));
}
