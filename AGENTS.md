# AGENTS.md

## How to reach the running game

Use these before writing an ad-hoc script to launch, poke at or screenshot a game.
They are built with the preset (`rake compile`, or `cmake --build --preset debug`) into
`build/<preset>/bin/`; run any of them with no argument for its usage.
Debug presets only: a release build has no agent channel.

```sh
B=build/debug/bin                       # build/debug-sfml/bin for SFML
$B/envs up a1                           # an env: its own copy of the game in .run/a1/game
                                        #   (default ../PSDKTechnicalDemo; --game DIR, $PSDK_GAME_DIR)
$B/litergss-run --env a1                # start it on this build, hidden window; returns once it answers
$B/litergss-agent --env a1 run state    # {"frame":..,"backend":"raylib","size":[320,240],"scene":"Scene_Map",..}
$B/litergss-agent --env a1 run shot x   # PNG of the current frame → .run/a1/run-NNNN/shots/x-<frame>.png
$B/litergss-agent --env a1 '$scene.class'   # eval Ruby in the game, between two frames
$B/envs stop a1                         # quit through the game's own close path; lists the files the run wrote
$B/envs ps                              # envs of this worktree and what runs in them
```

Drive and inspect it (scripts under `run`; `litergss-agent` alone lists them):

```sh
$B/litergss-agent --env a1 run press Down 4       # a key, as the keyboard sends it (events + polled state)
$B/litergss-agent --env a1 run pause              # hold the game: no game code runs
$B/litergss-agent --env a1 run step 3             # exactly 3 updates, then held again; next request answers after
$B/litergss-agent --env a1 run tree               # viewports and the drawables in each
$B/litergss-agent --env a1 run fps                # median / p95 frame times
$B/litergss-run --env a1 --dump-frames 60         # the presented frames (after scale + post-processing) → run-NNNN/frames/
```

Compare rendering across runtimes, with no game (scenarios in `tests/scenes/`):

```sh
$B/litergss-scene tests/scenes/sprites_viewports.rb --on raylib,sfml --diff sfml   # exit 1 over the threshold, diff PNG
cmake --build --preset debug --target litergss2-reference                          # once: the LiteRGSS2 oracle (../litergss2)
$B/envs up a1 --xvfb && $B/litergss-scene <scene> --env a1                         # vs LiteRGSS2 (needs Xvfb: no hidden window there)
```

Profile (Tracy build; tools built once with `cmake --build --preset debug-tracy --target tracy-tools`):

```sh
build/debug-tracy/bin/litergss-run --env a1 && build/debug-tracy/bin/litergss-profile --env a1 --seconds 10
```

- `litergss-agent` with no argument lists the scripts (`tools/agent/*.rb`); add one there rather
  than repeating a long eval. A script's last value is its JSON result; `args` holds its arguments.
- Each run gets `.run/<env>/run-NNNN/`: `stdout.log`, `stderr.log`, `logs/` (engine logs,
  `LITECGSS_LOG_DIR`), `shots/`, and `files-written.txt` after `envs stop`.
- Never stop a game with a signal: SIGTERM opens PSDK's error window and writes `Error.log`.
  `envs stop` quits it properly, and SIGKILLs only a game that no longer answers (and says so).
- Agent runs never touch the human's game folder: `litergss-run` only runs an env's copy.
- The channel itself: `litecgss/include/LiteCGSS/Agent/Channel.h` (engine, `LITECGSS_AGENT`
  CMake option, ON in debug presets) and `ext/LiteRGSS3/agent/` (the `eval` / `run` verbs). It
  opens only when `LITERGSS_AGENT` names a socket, which `litergss-run` sets.
- `step` makes the engine deterministic, not a game that reads the clock: part of PSDK's intro is
  timed, so it moves on by wall time across a pause.
- `envs up NAME --xvfb` gives the env a display of its own (needs `xvfb` installed); without it, runs
  use hidden windows on yours. LiteRGSS2 has no hidden window, so `litergss-scene` refuses it there.
- The tooling has its own test: `ctest --preset debug -R agent_tooling`.
- `tools/psdk-demo.sh` (`rake psdk`) is the human's launcher: visible window, in the game folder itself.
