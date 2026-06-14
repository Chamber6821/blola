#include <blola/blola.hpp>
#include <blola/directWrite_stdout.hpp>

int main() {
  blog("Hello World!");
  blog("My data: %d", 123);
}
