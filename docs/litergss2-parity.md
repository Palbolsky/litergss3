# LiteRGSS2 / LiteCGSS2 parity since 2026-05-01

Every commit made to LiteCGSS (`~/dev/litecgss`) and LiteRGSS2
(`~/dev/litergss2`) since 2026-05-01, and its counterpart in LiteRGSS3.
Ports keep LiteRGSS3's rules: backend code only under `Backend/`, bindings
only through LiteCGSS' public API.

Status: **ported** (commit given), **present** (already here before this
audit), **N/A** (doesn't apply to this repository), **pending**.

## LiteCGSS

| Commit(s) | Subject | Status |
|---|---|---|
| 3d42ab0, 17b124e, 4e1f00f, a41452f | shaders as-is, missing bits, destructive blit, common code | present |
| 3e3aaf9, a06d628 | VFS (PhysFS interface), IOLock | ported e8abff3 (see notes) |
| 58891db, 7276194, 44642c3, 2793f33, d11fb91, 4d797ce | shader preprocessor, NPOT factors | present (1972843) |
| 3d94877 | highp fragment precision; texture upload diagnostic | precision present; diagnostic ported 2f37055 |
| 0d8c91a, 70d50f7, 7276c6b | GLSL preprocessing on desktop | present (golden cases identical) |
| 4145a89, 52b3a0b, d3a06b3, d820b5a, a472bd1 | GL ES shader fixes | present (golden cases identical) |
| b38a311 | clear error when shaders aren't available | ported 9077cc1 |
| 1f2737b | SpriteMap batching per texture | ported 95c8bdd |
| ecf1729 | Tracy support | ported e4bce22 (off by default) |
| c95179c, d8e3cb1 | window origin, `originPixels` | ported fdd76d8 |
| 628a10d, d0a4616, 43d14a3, 83b74df, ca8717d, 3459962, 4d52e3b, 4116310, 8342891, dad4db8, bc90bcd, 716c81a, 04f519f, 38e11f0, caf2ad8, b404f38, 58452f1 | Android HostSurface, window preservation, EGL lifecycle | **pending** (design to agree, see below) |
| c0c7389, 29cbb83, 2d54e20, 4b53009 | CI, Rakefile cache refresh | N/A (CMake presets, Rake proxy) |
| 8a711d1, fd9e442, 8ff7bb1, f49f00c, 2c9e158 | merges | — |

## LiteRGSS2

| Commit(s) | Subject | Status |
|---|---|---|
| cfdbeeb | review changes (raylib branch) | present |
| d4b8196 | `Data_Wrap_Struct` warnings | N/A (TypedData everywhere) |
| a5dffd0, 413958b, a46b7df, 3116fe7 | Ruby VFS layer, then moved to the ruby-physfs gem | ported e8abff3: `AssetsArchive` / `AssetFile` / `AssetWriter` removed |
| 1f7f6c8 | `LiteRGSS.vfs_lock` / `vfs_lock=` | ported e8abff3 (physfs builds) |
| d541eaf, c33a76a, d1829f9, fce03a0, 66a0ee3 | preprocessor follow-ups | present |
| 32bc99a | `Shader::LITECGSS_SHADER_FIXED_PIPELINE` | ported 9077cc1 |
| c66e8db | Closed event no longer falls through to Resized | ported 9077cc1 |
| 9762adb, 08fcc9d, 86dea45 | `DisplayWindow#game_area`, `screen_width/height`, `Viewport#screen_space` | ported fdd76d8 |
| 044c274 | Tracy (`LiteRGSS_tracy` extension) | ported e4bce22 |
| 05c41be … a8ed2ab | Android HostSurface counterparts | **pending** |
| d6c11e6, 2d40083, 4b260da, ff7d5c7, f5babf4 | Docker / Ruby 3.4 / glibc build | N/A (packaging, litergss-everywhere) |
| 5d7d064, 2ef9dd3, 6891fad | stash entries | — |
| 49fd6da, d369a94 and merges | submodule bumps | — |

The runtime API of a physfs build matches the shipped LiteRGSS2 1.11.0
runtime exactly (every module, method and constant it defines exists here).
A default build lacks only `LiteRGSS.vfs_lock` / `vfs_lock=`, as a LiteRGSS2
built without physfs does.

## Deliberate differences

- **No `sf::InputStream` in the VFS.** LiteCGSS2's `PhysFSInputStream` /
  `vfs::openSFMLStream` would put an SFML type in a public header. Every
  backend here decodes assets from memory, so loaders use
  `vfs::loadFully`. The oracle's stream tests run over `vfs::openRead`'s
  `std::istream` instead.
- **Tracy is not fetched by default.** Zones go through a private
  `CGSS_PROFILE_ZONE` macro that compiles to nothing without
  `LITECGSS_USE_TRACY`.
- **raylib refuses textures beyond `GL_MAX_TEXTURE_SIZE`**, like SFML,
  rather than leaving the result to the driver.
- **SpriteMap batching keeps signed texture rects**: unlike the oracle's
  `abs()`, it doesn't drop flipped tiles.

## Fixed along the way

- `DisplayWindow#width` / `#height` became the window's pixel size (scale
  included) after a resize event, so on SFML at scale 2, PSDK's
  `Graphics.snap_to_bitmap` returned a 640×480 image (452c59a).
- The Ruby tests always loaded `build/lib`'s extension, whichever build
  ran them (ac9030b).
- The physfs build no longer configured on CMake 4 (e8abff3).

## Pending

- **Android HostSurface** (17 + 13 commits). It is Android platform code:
  only the windowing bridge (`sf::android::*`) is SFML-specific. Proposed
  layout:
  - a backend-neutral public API under `include/LiteCGSS/Platform/Android/`;
  - the attach / detach gate and bookkeeping in shared code;
  - the windowing calls through backend Ops: SFML implements them, raylib
    reports them unsupported.

  Waiting for agreement on this layout.

## Known gaps (not from these commits)

- raylib batches at most 4 texture units (`RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS`).
  Shaders sampling more than 3 extra textures (PSDK's `bk0`..`bk5`) lose
  some of them.
- `LiteRGSS::Bitmap` is the same class as `Image`; LiteRGSS2 has two
  classes. Every method LiteRGSS2 defines exists, but `Bitmap` /
  `Image` identity checks (`is_a?`, `==` on classes) differ.
- `test_sprite_constants`, `test_viewport_constants` and
  `test_window_constants` fail: they compare a `Rect` with an `Array`.
  The tests are wrong, not the bindings.
