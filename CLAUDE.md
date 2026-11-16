Always favor refactoring of similar existing code / API / algorithm before starting to write new code. It of course only applies if something is already existing.

In plan and agent modes, do not hesitate to ask questions if you need more information, in particular for huge complex tasks.

For the architecture / high complexity code related tasks, behave like a Senior Software engineer / an architect.

Challenge the solutions I am proposing (if there is an existing better way, don't challenge everything for the sake of challenge...).

The repo is a monorepo: `litecgss/` (C++ engine, no Ruby) and `ext/LiteRGSS3/` (Ruby bindings). CMake builds everything (`rake compile` proxies to the `debug` preset); an engine edit rebuilds the extension automatically.

Module boundary (enforced by the build, see README "LiteCGSS ↔ LiteRGSS boundary"): bindings only use LiteCGSS' public API (`<LiteCGSS/...>` from `litecgss/include/`). When a binding needs something the engine does not expose, add a backend-neutral public API to LiteCGSS (header in `include/LiteCGSS/`, implementation in `src/` over `ActiveBackend::Ops`) — never include backend headers, `CGSS_BACKEND_*` branches or `litecgss/src` paths in `ext/`.

Backend isolation: raylib / SFML code only lives in `litecgss/include/LiteCGSS/Backend/` and `litecgss/src/Backend/` (Ops in `*Backend.h`, backend-specific files in `src/Backend/Specializations/<backend>/`). Everything else in `litecgss` is backend-neutral and goes through `ActiveBackend::Ops` / `B::native_*` typedefs. Public headers must only forward-declare native types (never include a backend header).
