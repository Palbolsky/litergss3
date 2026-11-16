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
    PRESET=release rake compile   # any preset: debug, release, debug-sfml, debug-physfs, debug-tracy, litecgss
    rake test                     # LiteCGSS (C++) + LiteRGSS (Ruby) tests, through CTest
    rake clean

The tests open no window on the desktop: CTest runs the Ruby suite with
`LITERGSS_HIDDEN_WINDOW=1`, whose windows (and GL contexts) are real but never
shown. A display server is still needed (`xvfb-run rake test` without one).

The `debug-sfml` preset needs SFML **2.6** (SFML 3 is not supported), e.g. a
build of the SFML-opengl-es2 fork: point CMake at it with
`cmake --preset debug-sfml -DSFML_DIR=<sfml-install>/lib/cmake/SFML`.

Optional engine features (CMake options, off by default):

- `LITECGSS_USE_PHYSFS` (`debug-physfs` preset): assets load through
  PhysicsFS (`cgss::vfs`: mounted archives first, then the filesystem), and
  `LiteRGSS.vfs_lock=` shares PhysFS' lock with the physfs gem.
- `LITECGSS_USE_TRACY` (`debug-tracy` preset): instruments LiteCGSS for the
  [Tracy](https://github.com/wolfpld/tracy) profiler, v0.13.1. The
  extension is then `LiteRGSS_tracy`, next to the shared `TracyClient`
  every instrumented library of the process must share; the host must
  start the profiler before loading it (e.g. with tracy_ruby). Such a build
  skips the Ruby test suite for that reason.

Pick a generator with the `CMAKE_GENERATOR` environment variable
(e.g. `MinGW Makefiles` on Windows).

The extension is written to `build/lib/LiteRGSS.so`. To try it: `ruby ./start.rb`.

To play a PSDK game on it (instead of the LiteRGSS2 runtime of `Game.sh`):

    tools/psdk-demo.sh                      # ../PSDKTechnicalDemo, debug preset
    tools/psdk-demo.sh -g /path/to/game -p release -- debug   # args after -- go to Game.rb
    rake psdk                               # same, honours PRESET / PSDK_GAME_DIR

It builds the preset, stages the extension where PSDK's loader looks for it
(`build/psdk-gamedeps`) and runs `Game.rb` with the Ruby the extension was
compiled for. Audio stays silent (no SFMLAudio extension).

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
