#!/bin/sh
set -eu

root=$(cd "$(dirname "$0")/../.." && pwd)
image=supercollidaw-windows-builder

podman build -t "$image" "$root/packaging/windows"
podman run --rm -v "$root:/src" -w /src "$image" sh -euc '
    cmake -S . -B build-windows -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/usr/share/mingw/toolchain-mingw64.cmake -DPKG_CONFIG_EXECUTABLE=/usr/bin/x86_64-w64-mingw32-pkg-config -DCMAKE_PROJECT_supercollidaw_INCLUDE=/src/packaging/windows/static.cmake
    cmake --build build-windows --target supercollidaw_clap
    rm -rf build-windows/package build-windows/SuperColliDAW-windows-x64.zip
    mkdir -p build-windows/package
    x86_64-w64-mingw32-strip -o build-windows/package/SuperColliDAW.clap build-windows/src/SuperColliDAW.clap
    cp -r build-windows/src/SuperColliDAW build-windows/package/
    x86_64-w64-mingw32-strip build-windows/package/SuperColliDAW/plugins/*.scx
    cd build-windows/package && cmake -E tar cf ../SuperColliDAW-windows-x64.zip --format=zip SuperColliDAW.clap SuperColliDAW
'
echo "$root/build-windows/SuperColliDAW-windows-x64.zip"
