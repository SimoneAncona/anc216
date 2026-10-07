#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
build_production() {
    cmake -S . -B build/production -DANC216_WITH_SDL=ON "$@" \
        -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
    cmake --build build/production --parallel
    cmake --build build/production --target os --parallel
}

# Remove our flag while preserving every CMake argument and its quoting.
release=false
remaining=$#
while [ "$remaining" -gt 0 ]; do
    argument=$1
    shift
    case "$argument" in
        --release) release=true ;;
        *) set -- "$@" "$argument" ;;
    esac
    remaining=$((remaining - 1))
done
if [ "$release" = true ]; then
    build_production "$@"
    exit 0
fi
cmake -S . -B build -DANC216_WITH_SDL=ON "$@"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
