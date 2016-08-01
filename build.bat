@echo off

set warnings=-Wall -Wno-c++11-compat-deprecated-writable-strings
set flags=-O3
set emscripten_settings=-s AGGRESSIVE_VARIABLE_ELIMINATION=1 -s NO_FILESYSTEM=1 -s FULL_ES2=1
set emscripten_extra_settings=-s EXPORTED_RUNTIME_METHODS=[]
em++ emscripten.cpp -o ../output/test.html -s USE_SDL=2 %warnings% %flags% %emscripten_settings%


:: Build as a js library
:: em++ src/hello.cpp -o scripts/test.js --bind -s EXPORT_NAME="'mtt'" -s MODULARIZE=1