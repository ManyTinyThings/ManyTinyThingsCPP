#!/usr/bin/env bash
warnings="-Wall -Wno-c++11-compat-deprecated-writable-strings"
flags="-O3"
emscripten_settings="-s AGGRESSIVE_VARIABLE_ELIMINATION=1 -s NO_FILESYSTEM=1 -s USE_SDL=2"
em++ emscripten.cpp -o ../output/test.html $warnings $flags $emscripten_settings