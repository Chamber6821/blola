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

<img width="1383" height="128" alt="image" src="https://github.com/user-attachments/assets/a25082fe-17d1-4ce4-8a15-9214a5f76181" />

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

<img width="2958" height="579" alt="image" src="https://github.com/user-attachments/assets/2bbd73eb-3a9f-4efb-9067-af4816351184" />

If type mismatch:

```c++
#include <blola/blola.hpp>
#include <blola/directWrite_stdout.hpp>

int main() { blog("My data: %d", 'X'); }
```

Error message:

<img width="2396" height="459" alt="image" src="https://github.com/user-attachments/assets/9882df35-c12a-4116-a691-8ba173e0dbe9" />
