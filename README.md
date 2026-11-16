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

LiteRGSS reaches the engine only through the `LiteCGSS::engine` CMake
target: the public `litecgss/include/` tree (`<LiteCGSS/...>`) plus the
configuration defines the engine was built with. The engine's private
`litecgss/src/` tree is not on LiteRGSS' include path, and LiteCGSS never
sees Ruby.
