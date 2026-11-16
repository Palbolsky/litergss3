#!/usr/bin/env bash
# Start a PSDK game (default: the PSDKTechnicalDemo checkout next to this
# repository) on the locally built LiteRGSS3 instead of the LiteRGSS2 runtime.
#
#   tools/psdk-demo.sh [options] [-- game args...]
#
# Options:
#   -g, --game DIR      PSDK game folder (default: $PSDK_GAME_DIR, else
#                       ../PSDKTechnicalDemo next to this repository)
#   -p, --preset NAME   CMake preset to build and run (default: $PRESET, else debug)
#   -n, --no-build      skip the incremental build
#   -h, --help          show this help
# Anything after `--` (or any unknown argument) is passed to Game.rb, e.g.
# `tools/psdk-demo.sh -- debug`.
#
# How it works: PSDK's loader requires "$GAMEDEPS/ruby-dist/lib/LiteRGSS" on
# Unix. The script stages build/lib/LiteRGSS.so there (symlink, so rebuilds
# are picked up) and runs Game.rb with the Ruby the extension was compiled
# against (RUBY_EXECUTABLE from the preset's CMake cache): a native extension
# only loads in the Ruby version it was built for. Audio is silent unless a
# SFMLAudio / RubyFmod extension is also put in that folder.

set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GAME_DIR="${PSDK_GAME_DIR:-$REPO_DIR/../PSDKTechnicalDemo}"
PRESET_NAME="${PRESET:-debug}"
BUILD=1
GAME_ARGS=()

usage() { sed -n '2,/^$/{s/^# \{0,1\}//;p}' "${BASH_SOURCE[0]}"; }

while [[ $# -gt 0 ]]; do
	case "$1" in
		-g|--game)     GAME_DIR="$2"; shift 2 ;;
		-p|--preset)   PRESET_NAME="$2"; shift 2 ;;
		-n|--no-build) BUILD=0; shift ;;
		-h|--help)     usage; exit 0 ;;
		--)            shift; GAME_ARGS+=("$@"); break ;;
		*)             GAME_ARGS+=("$1"); shift ;;
	esac
done

die() { echo "psdk-demo: $*" >&2; exit 1; }

[[ -f "$GAME_DIR/Game.rb" ]] || die "no Game.rb in '$GAME_DIR' (use --game DIR or PSDK_GAME_DIR)"
GAME_DIR="$(cd "$GAME_DIR" && pwd)"

BINARY_DIR="$REPO_DIR/build/$PRESET_NAME"
if [[ $BUILD -eq 1 ]]; then
	cd "$REPO_DIR"
	[[ -f "$BINARY_DIR/CMakeCache.txt" ]] || cmake --preset "$PRESET_NAME"
	cmake --build --preset "$PRESET_NAME" --parallel --target LiteRGSS
fi
[[ -f "$BINARY_DIR/CMakeCache.txt" ]] || die "preset '$PRESET_NAME' is not configured (drop --no-build)"

cache_value() { sed -n "s/^$1:[A-Z]*=//p" "$BINARY_DIR/CMakeCache.txt"; }
RUBY_BIN="$(cache_value RUBY_EXECUTABLE)"
EXT_DIR="$(cache_value LITERGSS_EXT_OUTPUT_DIR)"
EXTENSION="$(ls "$EXT_DIR"/LiteRGSS.* 2>/dev/null | head -n1 || true)"
[[ -x "$RUBY_BIN" ]] || die "Ruby '$RUBY_BIN' from the CMake cache is not executable"
[[ -n "$EXTENSION" ]] || die "no LiteRGSS extension in '$EXT_DIR' (build first)"

# Staged dependency folder, laid out the way PSDK's Unix loader expects.
GAMEDEPS_DIR="$REPO_DIR/build/psdk-gamedeps"
mkdir -p "$GAMEDEPS_DIR/ruby-dist/lib"
ln -sfn "$EXTENSION" "$GAMEDEPS_DIR/ruby-dist/lib/$(basename "$EXTENSION")"

echo "psdk-demo: $GAME_DIR"
echo "psdk-demo: LiteRGSS3 $EXTENSION (preset $PRESET_NAME), $("$RUBY_BIN" -e 'print RUBY_DESCRIPTION')"

cd "$GAME_DIR"
export GAMEDEPS="$GAMEDEPS_DIR"
exec "$RUBY_BIN" Game.rb "${GAME_ARGS[@]}"
