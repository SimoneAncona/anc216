#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
exec ./build/production/emulator/anc216emu \
    --boot ./build/production/os/boot.bin \
    --insert 0x0100 ./build/production/os/system.rom \
    --insert-charmap ./build/production/os/charmap.bin \
    --insert-card 0x3000 ./build/production/os/disk0.afs \
    "$@"
