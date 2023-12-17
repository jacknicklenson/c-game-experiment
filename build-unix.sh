#!/bin/sh

if [ "$1" = "release" ]
then
  conan install . --output-folder=build/release --build=missing -s build_type=Release && \
  cd build/release && \
  meson setup --native-file conan_meson_native.ini ../.. meson-src && \
  meson compile -C meson-src && \
  cp -f ./meson-src/compile_commands.json ../.. && \
  cp -rf ../../assets ./meson-src/assets && \
  cd ../..
else
  conan install . --output-folder=build/debug --build=missing -s build_type=Debug && \
  cd build/debug && \
  meson setup --native-file conan_meson_native.ini ../.. meson-src && \
  meson compile -C meson-src && \
  cp -f ./meson-src/compile_commands.json ../.. && \
  cp -rf ../../assets ./meson-src/assets && \
  cd ../..
fi
