#!/usr/bin/env bash
# Start a PSDK game (default: the PSDKTechnicalDemo checkout next to this
# repository) on the locally built LiteRGSS3 instead of the LiteRGSS2 runtime.
#
#   tools/psdk-demo.sh [options] [-- game args...]
#
# Options:
#   -g, --game DIR      PSDK game folder (default: $PSDK_GAME_DIR, else
#                       ../PSDKTechnicalDemo next to this repository)
#   -b, --backend NAME  raylib or sfml: runs the debug preset of that backend
#                       (debug / debug-sfml); -p picks any other preset
#   -p, --preset NAME   CMake preset to build and run (default: $PRESET, else debug)
#   -n, --no-build      skip the incremental build
#   -h, --help          show this help
# Anything after `--` (or any unknown argument) is passed to Game.rb, e.g.
# `tools/psdk-demo.sh -b sfml -- debug`.
#
# The SFML preset needs SFML 2.6: vendor/sfml (see README), else $SFML_DIR
# (<sfml-install>/lib/cmake/SFML) at the first configure.
#
# The window title gets " - LiteRGSS3 <backend> (<preset>)" appended, to tell
# runs apart (LITERGSS_TITLE_SUFFIX, read by the extension).
#
# How it works: PSDK's loader requires "$GAMEDEPS/ruby-dist/lib/LiteRGSS" on
# Unix. The script stages the preset's extension (build/<preset>/lib) in
# build/<preset>/psdk-gamedeps (symlink, so rebuilds are picked up: one
# folder per preset, so backends can run side by side) and runs Game.rb
# with the Ruby the extension was compiled
# against (RUBY_EXECUTABLE from the preset's CMake cache): a native extension
# only loads in the Ruby version it was built for. Audio is silent unless a
# SFMLAudio / RubyFmod extension is also put in that folder.

set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GAME_DIR="${PSDK_GAME_DIR:-$REPO_DIR/../PSDKTechnicalDemo}"
PRESET_NAME="${PRESET:-debug}"
BACKEND=""
BUILD=1
GAME_ARGS=()

usage() { sed -n '2,/^$/{s/^# \{0,1\}//;p}' "${BASH_SOURCE[0]}"; }

while [[ $# -gt 0 ]]; do
	case "$1" in
		-g|--game)     GAME_DIR="$2"; shift 2 ;;
		-b|--backend)  BACKEND="$2"; shift 2 ;;
		-p|--preset)   PRESET_NAME="$2"; shift 2 ;;
		-n|--no-build) BUILD=0; shift ;;
		-h|--help)     usage; exit 0 ;;
		--)            shift; GAME_ARGS+=("$@"); break ;;
		*)             GAME_ARGS+=("$1"); shift ;;
	esac
done

die() { echo "psdk-demo: $*" >&2; exit 1; }

case "$BACKEND" in
	"")     ;;
	raylib) PRESET_NAME=debug ;;
	sfml)   PRESET_NAME=debug-sfml ;;
	*)      die "unknown backend '$BACKEND' (raylib or sfml)" ;;
esac

[[ -f "$GAME_DIR/Game.rb" ]] || die "no Game.rb in '$GAME_DIR' (use --game DIR or PSDK_GAME_DIR)"
GAME_DIR="$(cd "$GAME_DIR" && pwd)"

BINARY_DIR="$REPO_DIR/build/$PRESET_NAME"
if [[ $BUILD -eq 1 ]]; then
	cd "$REPO_DIR"
	if [[ ! -f "$BINARY_DIR/CMakeCache.txt" ]]; then
		CONFIGURE_ARGS=()
		[[ -n "${SFML_DIR:-}" ]] && CONFIGURE_ARGS+=("-DSFML_DIR=$SFML_DIR")
		cmake --preset "$PRESET_NAME" "${CONFIGURE_ARGS[@]}"
	fi
	cmake --build --preset "$PRESET_NAME" --parallel --target LiteRGSS
fi
[[ -f "$BINARY_DIR/CMakeCache.txt" ]] || die "preset '$PRESET_NAME' is not configured (drop --no-build)"

cache_value() { sed -n "s/^$1:[A-Z]*=//p" "$BINARY_DIR/CMakeCache.txt"; }
RUBY_BIN="$(cache_value RUBY_EXECUTABLE)"
BACKEND_NAME="$(cache_value CGSS_BACKEND)"
EXT_DIR="$(cache_value LITERGSS_EXT_OUTPUT_DIR)"
EXTENSION="$(ls "$EXT_DIR"/LiteRGSS.* 2>/dev/null | head -n1 || true)"
[[ -x "$RUBY_BIN" ]] || die "Ruby '$RUBY_BIN' from the CMake cache is not executable"
[[ -n "$EXTENSION" ]] || die "no LiteRGSS extension in '$EXT_DIR' (build first)"

# Staged dependency folder, laid out the way PSDK's Unix loader expects.
GAMEDEPS_DIR="$BINARY_DIR/psdk-gamedeps"
mkdir -p "$GAMEDEPS_DIR/ruby-dist/lib"
ln -sfn "$EXTENSION" "$GAMEDEPS_DIR/ruby-dist/lib/$(basename "$EXTENSION")"

echo "psdk-demo: $GAME_DIR"
echo "psdk-demo: LiteRGSS3 $EXTENSION (backend $BACKEND_NAME, preset $PRESET_NAME), $("$RUBY_BIN" -e 'print RUBY_DESCRIPTION')"

cd "$GAME_DIR"
export GAMEDEPS="$GAMEDEPS_DIR"
export LITERGSS_TITLE_SUFFIX=" - LiteRGSS3 $BACKEND_NAME ($PRESET_NAME)"
exec "$RUBY_BIN" Game.rb "${GAME_ARGS[@]}"
