@echo off

cd /D %~dp0

md build

cd build

conan install .. --build=missing --settings build_type=Debug --profile=../profiles/clang

set cmake_path=%~dp0
set build_path=%cmake_path%build
set ninja_path=%build_path%\ninja

rd /s /q %ninja_path%

cmake -G "Visual Studio 17 2022" -S %cmake_path% -B %build_path%\project
cmake -G "Ninja" -S %cmake_path% -B %ninja_path%

pause