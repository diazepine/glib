#include <glib.h>
#include <stdio.h>
#include <unistd.h>

static void exhaust_tls_keys (gboolean release_after) {
  // macOS TLS key limit is 512 (NOTE: the actual limit measured as 501)
  long max_keys = sysconf(_SC_THREAD_KEYS_MAX);
  g_print("max keys: %ld\n", max_keys);
  size_t ok = 0;
  pthread_key_t keys[max_keys]; // cap what you probe
  while (ok < sizeof(keys)/sizeof(keys[0])) {
    int r = pthread_key_create(&keys[ok], NULL);
    if (r != 0) {
      g_print("TLS key limit reached!\n");
      break; // EAGAIN or ENOMEM
    }
    ok++;
  }
  if (release_after) {
    for (size_t i = 0; i < ok; i++)
      pthread_key_delete(keys[i]);
  }
}

void glib_init(void);

int main(void) {

  // XXX NOTE: we need to explicitly call glib_init() here so that the
  // following g_print calls work
  glib_init();

  exhaust_tls_keys(FALSE);

  // call glib_deinit here so that we can replicate a glib_init failure due
  // to exhausted TLS keys
  glib_deinit();

  glib_init();

  return 0;
}
