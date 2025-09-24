#include <glib.h>
#include <stdio.h>


static void tls_dtor(gpointer data) {
    g_print("[dtor]  thread=%p  data=%s\n", g_thread_self(), (char *)data);
    g_free(data);
}

// GPrivate initilaized at compitle-time with a destructor
static GPrivate tls_key = G_PRIVATE_INIT(tls_dtor);

static gpointer worker(gpointer user_data) {
    const gchar *name = user_data;

    // firest read: should be NULL
    gpointer val = g_private_get(&tls_key);
    g_print("[%s] init get -> %p\n", name, val);

    // set thread-local string
    // g_strdup_printf similar to standard C sprintf()
    g_private_set(&tls_key, g_strdup_printf("hello from %s", name));

    // read string back
    g_print("[%s] after set -> %s\n", name, (char*) g_private_get(&tls_key));

    /* Replace value (invokes dtor on old value immediately) */
    g_private_replace(&tls_key, g_strdup_printf("updated by %s", name));
    g_print("[%s] after replace -> %s\n", name, (char*) g_private_get(&tls_key));

    // leaving the thread here:
    // this should trigger the tls destructor (tls_dtor)
    return NULL;
}

void glib_init(void);

int main(void) {
    // workaround for Frida fork's glib bulids not linked with an init
    // routine (e.g., -Wl,-init,_glib_init )
    glib_init();

    GThread *t1 = g_thread_new("t1", worker, "t1");
    GThread *t2 = g_thread_new("t2", worker, "t2");

    // the main thread has its own TLS slot (starting as NULL)
    g_print("[main] init get -> %p\n", g_private_get(&tls_key));
    g_private_set(&tls_key, g_strdup("main-state"));
    g_print("[main] after set -> %s\n", (char*) g_private_get(&tls_key));

    g_thread_join(t1);
    g_thread_join(t2);

    // clean up main thread's value manually by callin the destructor explicitely
    // ofc, when the process exit this will take place automatically
    g_private_replace(&tls_key, NULL);

    return 0;
}
