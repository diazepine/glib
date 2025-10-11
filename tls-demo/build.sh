#!/usr/bin/env bash
set -euo pipefail

# build & run against the GLib staged in builddir/_stage (ASan)
STAGE_ROOT="$(cd "$(dirname "$0")"/.. && pwd)/builddir/_stage"
cd "$(dirname "$0")"

export PKG_CONFIG_PATH="$STAGE_ROOT/usr/local/lib/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="$STAGE_ROOT"

function build_test() {
    # if you want to enable ASAN, just add the flag below in cc command
    #   -fsanitize=address \
    local src="$1"
    local out="$2"
    cc -g \
        "$src" -o "$out" \
        $(pkg-config --cflags glib-2.0) \
        -L"$STAGE_ROOT/usr/local/lib" -Wl,-search_paths_first \
        -lglib-2.0 $(pkg-config --libs glib-2.0 | sed 's/-lglib-2\.0//') \
        -Wl,-rpath,@loader_path/../builddir/_stage/usr/local/lib
    }

function use_local_glib_in_executable() {
    local bin="$1"
    install_name_tool -id @rpath/libglib-2.0.0.dylib \
        "$STAGE_ROOT/usr/local/lib/libglib-2.0.0.dylib" 2>/dev/null || true
    install_name_tool -change /usr/local/lib/libglib-2.0.0.dylib \
        @rpath/libglib-2.0.0.dylib "$bin" 2>/dev/null || true
    install_name_tool -change /usr/local/lib/libintl.8.dylib \
        @rpath/libintl.8.dylib "$bin" 2>/dev/null || true
    }

function show_glib_path_for_executable() {
    local bin="$1"
    otool -L "$bin" | grep libglib || true
}

function clean_test() {
    local out="$1"
    rm -f "$out"
}

# clean and build tests
clean_test tls_demo
clean_test test_glib_on_exhausted_tls_keys
clean_test test_gprivate
clean_test test_glib_on_exhausted_tls_keys_cold_start
clean_test test_glib_fallible_tls

build_test tls_demo.c tls_demo
build_test test_glib_on_exhausted_tls_keys.c test_glib_on_exhausted_tls_keys
build_test test_gprivate.c test_gprivate
build_test test_glib_on_exhausted_tls_keys_cold_start.c test_glib_on_exhausted_tls_keys_cold_start
build_test test_glib_fallible_tls.c test_glib_fallible_tls

use_local_glib_in_executable tls_demo
use_local_glib_in_executable test_glib_on_exhausted_tls_keys
use_local_glib_in_executable test_gprivate
use_local_glib_in_executable test_glib_on_exhausted_tls_keys_cold_start
use_local_glib_in_executable test_glib_fallible_tls

show_glib_path_for_executable tls_demo
show_glib_path_for_executable test_glib_on_exhausted_tls_keys
show_glib_path_for_executable test_gprivate
show_glib_path_for_executable test_glib_on_exhausted_tls_keys_cold_start
show_glib_path_for_executable test_glib_fallible_tls
