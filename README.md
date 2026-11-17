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
3. The external libraries, **provided** to the build: LiteRGSS never downloads
   them, it finds them (`CMAKE_PREFIX_PATH`, `<Package>_DIR`, the system), the
   way SFML finds its own dependencies:

   | Library | Needed by | Found as |
   |---|---|---|
   | raylib ≥ 6.0 | the raylib backend (default) | `raylib_DIR`, or a prebuilt tree with `-DRAYLIB_ROOT=` (e.g. `vendor/raylib/`) |
   | PhysicsFS 3 | `LITECGSS_USE_PHYSFS` (`debug-physfs`, Android) | `PhysFS_DIR` |
   | GoogleTest (with GMock) | the C++ tests (`LITECGSS_BUILD_TESTS`, debug presets) | `GTest_DIR` |
   | Tracy 0.13 client | `LITECGSS_USE_TRACY` (`debug-tracy`) | `Tracy_DIR`; built with `TRACY_ENABLE`, `TRACY_MANUAL_LIFETIME`, `TRACY_ONLY_LOCALHOST` (checked) |
   | SFML 2.6 | the SFML backend (`debug-sfml`) | `SFML_DIR` (e.g. `vendor/sfml/`) |

   The engine compiles against the headers of the raylib it links and refuses
   one older than 6.0 (configure time and `static_assert`).

   For a development machine, `tools/deps/build-deps.sh` builds the pinned
   versions into `vendor/deps/`, which the build uses automatically (a script
   you run once, outside the build):

       tools/deps/build-deps.sh                          # raylib, PhysicsFS, GoogleTest
       tools/deps/build-deps.sh --with gtest,tracy,tracy-tools   # + Tracy client, tracy-capture/-csvexport

   A **shared** raylib 6 hides the GLFW it embeds, which the engine needs to
   move the GL context between threads (a second window from a thread,
   PSDK's error screen); build such an install with GLFW exported
   (`-DCMAKE_C_FLAGS=-D_GLFW_BUILD_DLL`). A static raylib (build-deps.sh's)
   needs nothing.

## Compilation

    rake compile                  # = cmake --preset debug && cmake --build --preset debug
    PRESET=release rake compile   # any preset: debug, release, debug-sfml, debug-physfs, debug-tracy, litecgss
    rake test                     # LiteCGSS (C++) + LiteRGSS (Ruby) tests, through CTest
    rake clean

### Android (embedded)

LiteRGSS cross-compiles for Android as static libraries for a host that
embeds Ruby (litergss-everywhere): `libLiteRGSS.a` (`Init_LiteRGSS`, called by
the host), `libLiteCGSS_engine.a`, `libskalog.a` and the public headers
(`cmake --install`). The target Ruby can't run on the build machine: its
header folders are given instead (`LITERGSS_EMBEDDED`).

    export ANDROID_NDK=<ndk>                                   # r25b+
    export LITERGSS_RUBY_HDR_DIR=<target ruby>/include         # has ruby.h
    export LITERGSS_RUBY_ARCH_HDR_DIR=<target ruby>/include/<arch>  # has ruby/config.h
    tools/deps/build-deps.sh --android arm64-v8a               # raylib (Android, GL ES 3) + PhysicsFS
    cmake --preset android-arm64-v8a && cmake --build --preset android-arm64-v8a
    cmake --install build/android-arm64-v8a --prefix <staging>

The presets (`android-arm64-v8a`, `android-x86_64`) use
`vendor/deps-android-<ABI>/` when it exists; a packager passes its own prefix
(`-DCMAKE_PREFIX_PATH=<prefix> -DCMAKE_FIND_ROOT_PATH=<prefix>`: the NDK
toolchain only searches under `CMAKE_FIND_ROOT_PATH`). The agent tooling is
not built there.

The raylib backend needs **Ruby 3.2 or newer** (its one GL context follows the
Ruby thread holding the GVL, through Ruby's internal thread events): the
build refuses an older Ruby with it. The SFML backend has no such need.

The tests open no window on the desktop: CTest runs the Ruby suite with
`LITERGSS_HIDDEN_WINDOW=1`, whose windows (and GL contexts) are real but never
shown. A display server is still needed (`xvfb-run rake test` without one).

The `debug-sfml` preset needs SFML **2.6** (SFML 3 is not supported), e.g. a
build of the SFML-opengl-es2 fork. Install it in `vendor/sfml/` (ignored by
git, found automatically): `cmake --install <sfml-build> --prefix vendor/sfml`;
or point CMake at another install with
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

Each preset writes the extension to its own `build/<preset>/lib/` (`build/debug/lib/LiteRGSS.so` for
`rake compile`), so backends never overwrite each other. To try it: `ruby ./start.rb`.

To play a PSDK game on it (instead of the LiteRGSS2 runtime of `Game.sh`):

    tools/psdk-demo.sh                      # ../PSDKTechnicalDemo, debug preset (raylib)
    tools/psdk-demo.sh -b sfml              # the SFML backend (debug-sfml preset)
    tools/psdk-demo.sh -g /path/to/game -p release -- debug   # args after -- go to Game.rb
    rake psdk                               # same, honours PRESET / PSDK_GAME_DIR

It builds the preset, stages the extension where PSDK's loader looks for it
(`build/<preset>/psdk-gamedeps`) and runs `Game.rb` with the Ruby the extension was
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
