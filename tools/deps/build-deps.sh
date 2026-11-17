#!/usr/bin/env bash
# Builds LiteRGSS3's external dependencies into a prefix, for a developer
# machine or a quick Android check. LiteRGSS3's build never downloads
# anything: it finds its dependencies (CMAKE_PREFIX_PATH, <Pkg>_DIR); this
# script is one way to provide them, the way vendor/sfml is (README). A
# packager (litergss-everywhere) builds them its own way instead.
#
#   tools/deps/build-deps.sh [options]
#
#   --prefix DIR        install prefix (default: vendor/deps, which the
#                       build uses automatically for host builds)
#   --android ABI       cross-compile for Android (arm64-v8a, x86_64, ...):
#                       raylib's Android platform, GL ES 3; needs --ndk
#                       (default prefix then: vendor/deps-android-<ABI>)
#   --ndk DIR           Android NDK (default: $ANDROID_NDK)
#   --api N             Android API level (default: 26, as litergss-everywhere)
#   --with LIST         extras, comma-separated: gtest, tracy, tracy-tools
#                       (default for host builds: gtest)
#   -h, --help
#
# Always: raylib (static, position-independent) and PhysicsFS (static,
# position-independent). Versions are pinned below; sources are cloned into
# <prefix>/src.

set -euo pipefail

RAYLIB_TAG=6.0
PHYSFS_TAG=release-3.2.0
GTEST_TAG=v1.14.0
TRACY_TAG=v0.13.1

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PREFIX=""
ABI=""
NDK="${ANDROID_NDK:-}"
API=26
WITH=""

usage() { sed -n '2,/^$/{s/^# \{0,1\}//;p}' "${BASH_SOURCE[0]}"; }

while [[ $# -gt 0 ]]; do
	case "$1" in
		--prefix)  PREFIX="$2"; shift 2 ;;
		--android) ABI="$2"; shift 2 ;;
		--ndk)     NDK="$2"; shift 2 ;;
		--api)     API="$2"; shift 2 ;;
		--with)    WITH="$2"; shift 2 ;;
		-h|--help) usage; exit 0 ;;
		*) echo "build-deps: unknown option $1" >&2; usage >&2; exit 2 ;;
	esac
done

die() { echo "build-deps: $*" >&2; exit 1; }

CROSS=()
if [[ -n "$ABI" ]]; then
	[[ -n "$NDK" && -f "$NDK/build/cmake/android.toolchain.cmake" ]] || die "--android needs an NDK (--ndk DIR or ANDROID_NDK)"
	CROSS=(-DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" -DANDROID_ABI="$ABI"
	       -DANDROID_PLATFORM="android-$API" -DANDROID_STL=c++_shared)
	PREFIX="${PREFIX:-$REPO_DIR/vendor/deps-android-$ABI}"
else
	PREFIX="${PREFIX:-$REPO_DIR/vendor/deps}"
	WITH="${WITH:-gtest}"
fi
mkdir -p "$PREFIX/src"
PREFIX="$(cd "$PREFIX" && pwd)"
JOBS="$(nproc 2>/dev/null || echo 4)"

has() { [[ ",$WITH," == *",$1,"* ]]; }

fetch() { # name url tag
	local dir="$PREFIX/src/$1"
	if [[ ! -d "$dir/.git" ]]; then
		git -c advice.detachedHead=false clone --quiet --depth 1 --branch "$3" "$2" "$dir"
	fi
	echo "$dir"
}

build() { # name source-dir cmake-args...
	local name="$1" src="$2"; shift 2
	echo "== $name"
	cmake -S "$src" -B "$PREFIX/build/$name" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		-DCMAKE_POSITION_INDEPENDENT_CODE=ON "${CROSS[@]}" "$@" > "$PREFIX/build-$name.log" 2>&1 \
		|| die "$name: configure failed ($PREFIX/build-$name.log)"
	cmake --build "$PREFIX/build/$name" --parallel "$JOBS" >> "$PREFIX/build-$name.log" 2>&1 \
		|| die "$name: build failed ($PREFIX/build-$name.log)"
	cmake --install "$PREFIX/build/$name" >> "$PREFIX/build-$name.log" 2>&1 \
		|| die "$name: install failed ($PREFIX/build-$name.log)"
}

RAYLIB_ARGS=(-DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=OFF)
if [[ -n "$ABI" ]]; then
	RAYLIB_ARGS+=(-DPLATFORM=Android "-DOPENGL_VERSION=ES 3.0")
fi
build raylib "$(fetch raylib https://github.com/raysan5/raylib.git "$RAYLIB_TAG")" "${RAYLIB_ARGS[@]}"

# physfs 3.2.0 declares cmake_minimum_required(VERSION 2.8.x), which CMake 4 rejects.
build physfs "$(fetch physfs https://github.com/icculus/physfs.git "$PHYSFS_TAG")" \
	-DPHYSFS_BUILD_SHARED=OFF -DPHYSFS_BUILD_STATIC=ON -DPHYSFS_BUILD_TEST=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5

if has gtest; then
	build googletest "$(fetch googletest https://github.com/google/googletest.git "$GTEST_TAG")" \
		-DBUILD_GMOCK=ON -DINSTALL_GTEST=ON
fi

if has tracy || has tracy-tools; then
	TRACY_SRC="$(fetch tracy https://github.com/wolfpld/tracy.git "$TRACY_TAG")"
fi
if has tracy; then
	# The client every instrumented library of the process shares: shared, and
	# the setup LiteCGSS checks for (litecgss/CMakeLists.txt).
	build tracy "$TRACY_SRC" -DBUILD_SHARED_LIBS=ON -DTRACY_STATIC=OFF -DTRACY_ENABLE=ON -DTRACY_ON_DEMAND=ON \
		-DTRACY_DELAYED_INIT=ON -DTRACY_MANUAL_LIFETIME=ON -DTRACY_ONLY_LOCALHOST=ON
fi
if has tracy-tools; then
	[[ -z "$ABI" ]] || die "tracy-tools are host programs: build them without --android"
	# Tracy's own build downloads what these programs need (its CPM); no file dialog (needs GTK).
	for tool in capture csvexport; do
		build "tracy-$tool" "$TRACY_SRC/$tool" -DNO_ISA_EXTENSIONS=ON -DNO_FILESELECTOR=ON
		install -m 755 "$PREFIX/build/tracy-$tool/tracy-$tool" "$PREFIX/bin/tracy-$tool" 2>/dev/null \
			|| { mkdir -p "$PREFIX/bin"; install -m 755 "$PREFIX/build/tracy-$tool/tracy-$tool" "$PREFIX/bin/"; }
	done
fi

echo "installed in $PREFIX"
