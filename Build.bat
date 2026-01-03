@echo off

set cur_path=%~dp0
set build_path=%cur_path%build
set ninja_path=%build_path%\ninja
set vsvarsall_path=E:\Programs\VisualStudio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat
set asan_dll_path=E:\Programs\VisualStudio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\clang_rt.asan_dynamic-x86_64.dll

call %vsvarsall_path% x64
cd /D %ninja_path%
ninja

copy %asan_dll_path% %ninja_path%
