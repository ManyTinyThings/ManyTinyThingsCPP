#!/usr/bin/env bash
warnings="-Wall -Wno-c++11-compat-deprecated-writable-strings"
flags="-O3"
emscripten_settings="-s AGGRESSIVE_VARIABLE_ELIMINATION=1 -s NO_FILESYSTEM=1 -s USE_SDL=2"
em++ many_tiny_things.cpp -o emscripten_output/index.html $warnings $flags $emscripten_settings