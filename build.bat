@echo off

if exist build_x64 rd /s /q build_x64

mkdir build_x64
cmake -S . -B build_x64
cmake --build build_x64 --target ALL_BUILD --config Release

echo Building completed
