#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
cmake -S . -B build -DANC216_WITH_SDL=ON "$@"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
