Notes
===

These notes are for changes that need to be sent back upstream.

- `ext/sushi/src/sushi/framebuffer.cpp` needs to `#include <algorithm>` for `std::min` and `std::max`.
- `CMakeLists.txt` and `ext/soloud/CMakeLists.txt` need to use consistent methods of obtaining package `SDL2`.
- `PkgConfig` is not easily available when using Visual Studio, it should be avoided when possible (both `PkgConfig` and Visual Studio).
- Ginseng needs a `.exists(eid)` method.
