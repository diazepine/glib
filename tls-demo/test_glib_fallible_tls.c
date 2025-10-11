
#define _GNU_SOURCE
#include <glib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

static void exhaust_tls_keys(gboolean release_after)
{
  long max_keys = sysconf(_SC_THREAD_KEYS_MAX);
  g_print("[test] PTHREAD_KEYS_MAX: %ld\n", max_keys);

  size_t cap = (size_t)(max_keys > 0 ? max_keys : 1024);
  if (cap > 4096) cap = 4096; /* safety bound */

  pthread_key_t *keys = g_new0(pthread_key_t, cap);
  size_t ok = 0;

  while (ok < cap) {
    int r = pthread_key_create(&keys[ok], NULL);
    if (r != 0) {
      g_print("[test] TLS key limit reached after %zu keys\n", ok);
      break;
    }
    ok++;
  }

  if (release_after) {
    for (size_t i = 0; i < ok; i++)
      pthread_key_delete(keys[i]);
  }
  g_free(keys);
}

typedef struct {
  GMutex m;
  GCond  c;
  gboolean ready;
  gboolean task_done;
  GMainContext *ctx;  /* worker-owned */
  GMainLoop    *loop; /* worker-owned */
} Shared;

static gpointer worker_thread(gpointer data)
{
  Shared *S = (Shared *)data;

  GMainContext *ctx = g_main_context_new();
  GMainLoop *loop = g_main_loop_new(ctx, FALSE);

  g_main_context_push_thread_default(ctx);

  g_mutex_lock(&S->m);
  S->ctx = ctx;
  S->loop = loop;
  S->ready = TRUE;
  g_cond_signal(&S->c);
  g_mutex_unlock(&S->m);

  g_main_loop_run(loop);

  g_main_context_pop_thread_default(ctx);
  g_main_loop_unref(loop);
  g_main_context_unref(ctx);
  return NULL;
}

static gboolean worker_task_cb(gpointer user_data)
{
  Shared *S = (Shared *)user_data;
  g_print("[worker] task executing on worker GMainContext\n");
  g_mutex_lock(&S->m);
  S->task_done = TRUE;
  g_cond_signal(&S->c);
  g_mutex_unlock(&S->m);
  return G_SOURCE_REMOVE;
}

static gboolean quit_loop_cb(gpointer user_data)
{
  Shared *S = (Shared *)user_data;
  g_main_loop_quit(S->loop);
  return G_SOURCE_REMOVE;
}

int main(void)
{
  exhaust_tls_keys(FALSE);

  Shared S = {0};
  g_mutex_init(&S.m);
  g_cond_init(&S.c);

  GThread *thr = g_thread_new("worker-loop", worker_thread, &S);

  g_mutex_lock(&S.m);
  while (!S.ready)
    g_cond_wait(&S.c, &S.m);
  GMainContext *ctx = S.ctx;
  g_mutex_unlock(&S.m);

  g_assert_nonnull(ctx);

  g_main_context_invoke_full(ctx, G_PRIORITY_DEFAULT, worker_task_cb, &S, NULL);

  g_mutex_lock(&S.m);
  while (!S.task_done)              /* <-- fixed */
    g_cond_wait(&S.c, &S.m);
  g_mutex_unlock(&S.m);

  g_main_context_invoke_full(ctx, G_PRIORITY_DEFAULT, quit_loop_cb, &S, NULL);
  g_thread_join(thr);

  g_mutex_clear(&S.m);
  g_cond_clear(&S.c);

  g_print("[test] done\n");
  return 0;
}
