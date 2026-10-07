#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
exec ./build.sh --release "$@"
