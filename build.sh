#!/bin/bash

source .venv/bin/activate

rm -rf builddir/_stage

meson setup builddir --native-file native-3.11.ini
meson compile -C builddir

DESTDIR="$PWD/builddir/_stage" ninja -C builddir install

ls -l builddir/_stage/usr/local/lib/libglib-2.0.0.dylib
ls -l builddir/_stage/usr/local/lib/pkgconfig/glib-2.0.pc
