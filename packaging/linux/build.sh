#!/bin/sh
set -eu

root=$(cd "$(dirname "$0")/../.." && pwd)
image=supercollidaw-linux-builder

podman build -t "$image" "$root/packaging/linux"
podman run --rm -v "$root:/src" -w /src "$image" sh -euc '
    . /opt/rh/gcc-toolset-14/enable
    cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build-linux --target supercollidaw_clap
    rm -rf build-linux/package build-linux/SuperColliDAW-linux-x64.tar.gz
    mkdir -p build-linux/package
    strip -o build-linux/package/SuperColliDAW.clap build-linux/src/SuperColliDAW.clap
    cp -r build-linux/src/SuperColliDAW build-linux/package/
    cd build-linux/package && cmake -E tar czf ../SuperColliDAW-linux-x64.tar.gz SuperColliDAW.clap SuperColliDAW
'
echo "$root/build-linux/SuperColliDAW-linux-x64.tar.gz"
