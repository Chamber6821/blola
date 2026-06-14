# Blola

__Blola__ is C++ binary logging library for embedded systems.

Library inspired by [Trice](https://github.com/rokath/trice) but has more friendly CLI and no code modifications.

## Requirements

to build:

- C++ 23
- Some transport for logs ([SEGGER RTT](https://github.com/SEGGERMicro/RTT) integration already implemented)

to use host script:

- Python 3

## Usage

### Just log

```c++
#include <blola/blola.hpp>
#include <blola/directWrite_stdout.hpp>

int main() {
  blog("Hello World!");
  blog("My data: %d", 123);
}
```

Run

```
make run-just_log
```

### Error messages

If you don't implement `directWrite(...)` (or don't use supplied implementation)

```c++
#include <blola/blola.hpp>

int main() {
  blog("Hello World!");
  blog("My data: %d", 123);
}
```

Error message:

(insert image on github)

If type mismatch:

```c++
#include <blola/blola.hpp>
#include <blola/directWrite_stdout.hpp>

int main() { blog("My data: %d", 'X'); }
```

Error message:

(insert image on github)
