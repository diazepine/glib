
#!/usr/bin/env bash
set -euo pipefail

# build & run against the GLib staged in builddir/_stage (ASan)
STAGE_ROOT="$(cd "$(dirname "$0")"/.. && pwd)/builddir/_stage"
cd "$(dirname "$0")"

export PKG_CONFIG_PATH="$STAGE_ROOT/usr/local/lib/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="$STAGE_ROOT"

# if you want to disable ASAN, just remove:
# -fsanitize=address \
rm -f tls_demo
cc -g \
    -fsanitize=address \
    tls_demo.c -o tls_demo \
  $(pkg-config --cflags glib-2.0) \
  -L"$STAGE_ROOT/usr/local/lib" -Wl,-search_paths_first \
  -lglib-2.0 $(pkg-config --libs glib-2.0 | sed 's/-lglib-2\.0//') \
  -Wl,-rpath,@loader_path/../builddir/_stage/usr/local/lib

install_name_tool -id @rpath/libglib-2.0.0.dylib \
  "$STAGE_ROOT/usr/local/lib/libglib-2.0.0.dylib" 2>/dev/null || true
install_name_tool -change /usr/local/lib/libglib-2.0.0.dylib \
  @rpath/libglib-2.0.0.dylib ./tls_demo 2>/dev/null || true
install_name_tool -change /usr/local/lib/libintl.8.dylib \
  @rpath/libintl.8.dylib ./tls_demo 2>/dev/null || true

otool -L ./tls_demo | grep libglib || true
