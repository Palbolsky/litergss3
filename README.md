# LiteRGSS3

Monorepo:

| Folder           | What                                                                  |
|------------------|-----------------------------------------------------------------------|
| `litecgss/`      | LiteCGSS, the C++ engine. No Ruby involved — see `litecgss/Readme.md` |
| `ext/LiteRGSS3/` | LiteRGSS, the Ruby bindings over LiteCGSS                             |
| `tests/`         | Ruby test suite                                                       |

The build system is CMake (presets in `CMakePresets.json`); `rake` is a thin
wrapper over it.

## Before compilation

1. Ruby with its headers (on Windows: Ruby with Devkit, then `ridk enable`)
2. CMake ≥ 3.21 and a C++17 compiler
3. raylib, either:
   - installed system-wide (`find_package(raylib)` must find it), or
   - a prebuilt tree extracted in `vendor/raylib/` (`include/` + `lib/`,
     e.g. raylib-5.5_win32_mingw-w64 on Windows)

## Compilation

    rake compile                  # = cmake --preset debug && cmake --build --preset debug
    PRESET=release rake compile   # any preset: debug, release, debug-sfml, litecgss
    rake test                     # LiteCGSS (C++) + LiteRGSS (Ruby) tests, through CTest
    rake clean

Pick a generator with the `CMAKE_GENERATOR` environment variable
(e.g. `MinGW Makefiles` on Windows).

The extension is written to `build/lib/LiteRGSS.so`. To try it: `ruby ./start.rb`.

## LiteCGSS ↔ LiteRGSS boundary

Dependencies only go one way: LiteRGSS → LiteCGSS. It is enforced by the
build, not by convention:

- LiteRGSS links `LiteCGSS::engine` only. That target exposes the public
  `litecgss/include/` tree (`<LiteCGSS/...>`) and the configuration defines
  the engine was compiled with. The private `litecgss/src/` tree is not on
  LiteRGSS' include path, so including a private header fails to compile.
- LiteCGSS has no Ruby include path and knows nothing about LiteRGSS.
- The graphics backend (raylib, SFML) is private to the engine: it is linked
  through the internal `LiteCGSS_backend` target, public headers only
  forward-declare native types, so no backend header nor symbol reaches a
  consumer. Switching backends (`CGSS_BACKEND`) never touches consumer code.
- `cmake/CheckBoundaries.cmake` runs on every build (`check_boundaries`
  target; also `rake lint`) and fails it when:
  - a binding includes a backend header (raylib, SFML, GLFW, or
    `LiteCGSS/Backend/*`), branches on `CGSS_BACKEND_*` or names a backend
    type: add a backend-neutral API to LiteCGSS instead;
  - LiteCGSS code outside `include/LiteCGSS/Backend/` and `src/Backend/`
    does any of the above: backend code lives in the backend's `*Backend.h`
    Ops or in `src/Backend/Specializations/<backend>/`;
  - `ext/LiteRGSS3` CMake reaches past `LiteCGSS::engine` (`litecgss/src`,
    `LiteCGSS::extension_spi`);
  - LiteCGSS includes Ruby or LiteRGSS headers;
  - a public LiteCGSS header includes a private one.

Header conventions: `"LiteCGSS/..."` always means public API; private engine
headers are included without prefix (`"Backend/ActiveBackend.h"`), and only
from inside `litecgss/`.

`LiteCGSS::extension_spi` is the one sanctioned escape hatch to native backend
handles (`LiteCGSS/Extension/NativeAccess.h`), for out-of-tree plugins.
