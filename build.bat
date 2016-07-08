@echo off

if exist build_x64 rd /s /q build_x64
if exist build_x86 rd /s /q build_x86

mkdir build_x64
cd build_x64
cmake -G "Visual Studio 14 2015 Win64" ..
cd ..

mkdir build_x86
cd build_x86
cmake -G "Visual Studio 14 2015" ..
cd ..

echo Building completed
