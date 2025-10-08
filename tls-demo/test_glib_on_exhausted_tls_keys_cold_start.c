#include <glib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static void exhaust_tls_keys (gboolean release_after) {
  // macOS TLS key limit is 512 (NOTE: the actual limit measured as 501)
  long max_keys = sysconf(_SC_THREAD_KEYS_MAX);
  printf("max keys: %ld\n", max_keys);
  size_t ok = 0;
  pthread_key_t keys[max_keys]; // cap what you probe
  while (ok < sizeof(keys)/sizeof(keys[0])) {
    int r = pthread_key_create(&keys[ok], NULL);
    if (r != 0) {
      printf("TLS key limit reached!\n");
      break; // EAGAIN or ENOMEM
    }
    ok++;
  }
  if (release_after) {
    for (size_t i = 0; i < ok; i++)
      pthread_key_delete(keys[i]);
  }
}

/* one-shot tls failure callback */
static void on_tls_failure(void *ud) {
  (void)ud;
  fprintf(stderr, "[test] on_tls_callback_failure() fired\n");
}

int main(void) {
  extern gboolean glib_is_available (void);
  extern void glib_set_failure_callback (void (*cb)(void*), void *);

  glib_set_failure_callback(on_tls_failure, NULL);

  printf("[test] 1) exhaust TLS first (no GLib used yet)\n");
  exhaust_tls_keys(0);
  printf("[test]    glib_is_tls_available now: %d\n", glib_is_available());

  printf("[test] 2) call glib_init() — should be a safe no-op\n");
  glib_init();
  printf("[test] 3) process should not crash\n");
  printf("[test]    glib_is_tls_available now: %d\n", glib_is_available());
  return 0;
}
