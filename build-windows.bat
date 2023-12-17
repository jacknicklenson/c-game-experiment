@echo off

setlocal EnableDelayedExpansion

IF "%1"=="release" (
conan install . --output-folder=build\release --build=missing -s build_type=Release &&^
cd build\release &&^
meson setup --native-file conan_meson_native.ini ..\.. meson-src &&^
meson compile -C meson-src &&^
Xcopy .\meson-src\compile_commands.json ..\.. /Y/D &&^
Xcopy ..\..\assets .\meson-src\assets /S/E/C/I/Y/D &&^
cd ..\..
) ELSE (
conan install . --output-folder=build\debug --build=missing -s build_type=Debug &&^
cd build\debug &&^
meson setup --native-file conan_meson_native.ini ..\.. meson-src &&^
meson compile -C meson-src &&^
Xcopy .\meson-src\compile_commands.json ..\.. /Y/D &&^
Xcopy ..\..\assets .\meson-src\assets /S/E/C/I/Y/D &&^
cd ..\..
)
