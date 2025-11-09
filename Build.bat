@echo off

set cur_path=%~dp0
set build_path=%cur_path%build
set ninja_path=%build_path%\ninja
set vsvarsall_path=E:\Programs\VisualStudio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat

call %vsvarsall_path% x64
cd /D %ninja_path%
ninja