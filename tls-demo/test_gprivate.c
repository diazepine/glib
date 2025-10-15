
#include <glib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static void
exhaust_tls_keys (gboolean release_after)
{
  long max_keys = sysconf(_SC_THREAD_KEYS_MAX);
  printf("[test] PTHREAD_KEYS_MAX: %ld\n", max_keys);

  size_t cap = (size_t) (max_keys > 0 ? max_keys : 1024);
  if (cap > 2048) cap = 2048; /* safety bound */

  pthread_key_t *keys = g_new0 (pthread_key_t, cap);
  size_t ok = 0;

  while (ok < cap) {
    int r = pthread_key_create (&keys[ok], NULL);
    if (r != 0) {
      printf("[test] TLS key limit reached after %zu keys\n", ok);
      break;
    }
    ok++;
  }

  if (release_after) {
    for (size_t i = 0; i < ok; i++)
      pthread_key_delete (keys[i]);
  }
  g_free (keys);
}

/* on-failure callback: do NOT call GLib inside. */
static void
on_tls_failure (void *ud)
{
  (void) ud;
  fprintf (stderr, "[test] on_tls_failure() fired\n");
}

int
main (void)
{
  /* link the new symbols */
  extern gboolean glib_is_available (void);
  extern void glib_set_failure_callback (void (*cb)(void*), void *);

  glib_set_failure_callback (on_tls_failure, NULL);

  printf("[test] 1) glib init\n");
  glib_init();
  printf("[test]    glib_is_available: %d\n", glib_is_available());

  printf("[test] 2) exhaust TLS keys\n");
  exhaust_tls_keys(FALSE);

  printf("[test] 3) glib init\n");
  glib_init();
  printf("[test]    glib_is_available: %d\n (should not be!)", glib_is_available());

  if (!glib_is_available())
    printf("[test]    TLS now unavailable (expected!)\n");

  printf("[test] 4) touch thread-default APIs (should not abort)\n");
  GMainContext *def = g_main_context_get_thread_default();
  printf("[test]    get_thread_default -> %p\n", (void*) def);

  GMainContext *tmp = g_main_context_new();
  g_main_context_push_thread_default(tmp);  /* early-return in no-TLS */
  g_main_context_pop_thread_default(tmp);   /* early-return in no-TLS */
  g_main_context_unref(tmp);

  printf("[test] 5) done; no abort occurred (expected!)\n");
  return 0;
}
