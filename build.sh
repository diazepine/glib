#!/bin/bash

FALLIBLE_GPRIVATE=""
if [ "$1" == "--fallible-gprivate" ]; then
  FALLIBLE_GPRIVATE="-Dfallible_gprivate=true"
  echo "Building with fallible gprivate"
else
  echo "Usage: $0 [--fallible-gprivate]"
fi


source .venv/bin/activate

rm -rf builddir/_stage

meson setup builddir --native-file native-3.11.ini $FALLIBLE_GPRIVATE
meson compile -C builddir

DESTDIR="$PWD/builddir/_stage" ninja -C builddir install

ls -l builddir/_stage/usr/local/lib/libglib-2.0.0.dylib
ls -l builddir/_stage/usr/local/lib/pkgconfig/glib-2.0.pc
