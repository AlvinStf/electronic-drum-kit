#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD=$(mktemp -d)
trap 'rm -rf "$BUILD"' EXIT HUP INT TERM
CXX=${CXX:-g++}
python3 "$ROOT/tests/check_sources.py"
python3 "$ROOT/tests/test_converter.py"
for NAME in test_engine test_midi_integration test_settings_ui; do
  "$CXX" -std=c++17 -DDRUM_UI_HOST_TEST=1 -Wall -Wextra -Werror -Wno-unused-variable \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I"$ROOT/tests/stubs" -I"$ROOT/DrumMIDI_ST7789" \
    "$ROOT/tests/$NAME.cpp" -o "$BUILD/$NAME"
  "$BUILD/$NAME"
done
