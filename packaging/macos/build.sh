#!/bin/sh
set -eu

root=$(cd "$(dirname "$0")/../.." && pwd)
build="$root/build-macos"
deps="$root/build-macos-deps"
# One plugin for Apple Silicon and Intel; macOS 11 is the first that runs on Apple Silicon.
archs="arm64;x86_64"
target=11.0

# TODO: build libogg, libvorbis, flac and opus statically too, so Buffer.read handles the same formats as on Linux.
# libsndfile 1.2.2 asks for CMake 3.1, which CMake 4 refuses without a policy minimum.
LIBSNDFILE_VERSION=1.2.2
if [ ! -f "$deps/lib/libsndfile.a" ]; then
    mkdir -p "$deps/src"
    curl -fsSL "https://github.com/libsndfile/libsndfile/releases/download/$LIBSNDFILE_VERSION/libsndfile-$LIBSNDFILE_VERSION.tar.xz" | tar -xJ -C "$deps/src"
    cmake -S "$deps/src/libsndfile-$LIBSNDFILE_VERSION" -B "$deps/src/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_OSX_ARCHITECTURES="$archs" -DCMAKE_OSX_DEPLOYMENT_TARGET=$target -DCMAKE_INSTALL_PREFIX="$deps" -DBUILD_SHARED_LIBS=OFF -DENABLE_EXTERNAL_LIBS=OFF -DENABLE_MPEG=OFF -DBUILD_PROGRAMS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF -DENABLE_CPACK=OFF -DINSTALL_MANPAGES=OFF
    cmake --build "$deps/src/build"
    cmake --install "$deps/src/build"
fi

export PKG_CONFIG_PATH="$deps/lib/pkgconfig"
cmake -S "$root" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="$archs" -DCMAKE_OSX_DEPLOYMENT_TARGET=$target
cmake --build "$build" --target supercollidaw_clap

package="$build/package"
rm -rf "$package" "$build/SuperColliDAW-macos.zip"
mkdir -p "$package"
cp -R "$build/src/SuperColliDAW.clap" "$build/src/SuperColliDAW" "$package/"
strip -x "$package/SuperColliDAW.clap/Contents/MacOS/SuperColliDAW" "$package/SuperColliDAW/plugins/"*.scx
# Apple Silicon only runs signed code, and stripping drops the linker's signature. An ad hoc signature needs no Apple account.
codesign --force --sign - "$package/SuperColliDAW.clap" "$package/SuperColliDAW/plugins/"*.scx
cd "$package" && zip -qr ../SuperColliDAW-macos.zip SuperColliDAW.clap SuperColliDAW
echo "$build/SuperColliDAW-macos.zip"
