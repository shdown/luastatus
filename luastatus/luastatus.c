#include <assert.h>
#include <lua.h>
#include <lauxlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <locale.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include "include/loglevel.h"
#include "include/barlib_data.h"
#include "include/plugin_data.h"
#include "libls/sprintf_utils.h"
#include "libls/compdep.h"
#include "libls/alloc_utils.h"
#include "libls/errno_utils.h"
#include "libls/vector.h"
#include "barlib.h"
#include "widget.h"
#include "log.h"
#include "pth_check.h"
#include "check_lua_call.h"
#include "load_by_name.h"
#include "taints.h"
#include "lua_libs.h"
#include "config.generated.h"
#include "version.generated.h"

Barlib barlib;
Widget *widgets;
size_t nwidgets;

#define LOCK_B()   PTH_CHECK(pthread_mutex_lock(&barlib.set_mtx))
#define UNLOCK_B() PTH_CHECK(pthread_mutex_unlock(&barlib.set_mtx))

#define LOCK_L(W_)   PTH_CHECK(pthread_mutex_lock(&(W_)->lua_mtx))
#define UNLOCK_L(W_) PTH_CHECK(pthread_mutex_unlock(&(W_)->lua_mtx))

#define LOCK_L_OR_S(W_)   PTH_CHECK(pthread_mutex_lock(widget_event_lua_mutex(W_)))
#define UNLOCK_L_OR_S(W_) PTH_CHECK(pthread_mutex_unlock(widget_event_lua_mutex(W_)))

#define WIDGET_INDEX(W_) ((W_) - widgets)

LS_ATTR_NORETURN
void
fatal_error_reported(void)
{
    _exit(EXIT_FAILURE);
}

void
external_logf(void *userdata, LuastatusLogLevel level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    if (userdata) {
        Widget *w = userdata; // w->state == WIDGET_STATE_INITED
        char who[1024];
        ls_ssnprintf(who, sizeof(who), "%s@%s", w->plugin.name, w->filename);
        common_vlogf(level, who, fmt, vl);
    } else {
        common_vlogf(level, "barlib", fmt, vl);
    }
    va_end(vl);
}

void
set_error_unlocked(size_t widget_idx)
{
    switch (barlib.iface.set_error(&barlib.data, widget_idx)) {
    case LUASTATUS_BARLIB_SET_ERROR_RESULT_OK:
        break;
    case LUASTATUS_BARLIB_SET_ERROR_RESULT_FATAL_ERR:
        internal_logf(LUASTATUS_FATAL, "barlib's set_error() reported fatal error");
        fatal_error_reported();
        break;
    }
}

lua_State *
plugin_call_begin(void *userdata)
{
    Widget *w = userdata; // w->state == WIDGET_STATE_INITED

    LOCK_L(w);

    assert(lua_gettop(w->L) == 0); // w->L: -
    lua_rawgeti(w->L, LUA_REGISTRYINDEX, w->lua_ref_cb); // w->L: cb
    return w->L;
}

void
plugin_call_end(void *userdata)
{
    Widget *w = userdata; // w->state == WIDGET_STATE_INITED
    lua_State *L = w->L;

    assert(lua_gettop(L) == 2); // L: cb data

    bool r = check_lua_call(L, lua_pcall(L, 1, 1, 0));

    LOCK_B();

    size_t widget_idx = WIDGET_INDEX(w);
    if (!r) {
        // L: -
        set_error_unlocked(widget_idx);
    } else {
        // L: result
        switch (barlib.iface.set(&barlib.data, L, widget_idx)) {
        case LUASTATUS_BARLIB_SET_RESULT_OK:
            // L: result
            break;
        case LUASTATUS_BARLIB_SET_RESULT_NONFATAL_ERR:
            // L: ?
            set_error_unlocked(widget_idx);
            break;
        case LUASTATUS_BARLIB_SET_RESULT_FATAL_ERR:
            // L: ?
            internal_logf(LUASTATUS_FATAL, "barlib's set() reported fatal error");
            fatal_error_reported();
            break;
        }
        lua_pop(L, lua_gettop(L)); // L: -
    }

    UNLOCK_B();
    UNLOCK_L(w);
}

void *
widget_thread(void *userdata)
{
    Widget *w = userdata; // w->state == WIDGET_STATE_INITED

    w->plugin.iface.run(&w->data, plugin_call_begin, plugin_call_end);
    internal_logf(LUASTATUS_WARN, "plugin's (%s) run() for widget '%s' has returned",
                  w->plugin.name, w->filename);

    LOCK_B();
    set_error_unlocked(WIDGET_INDEX(w));
    UNLOCK_B();

    // We can't even uninit the widget here, because functions registered by plugin (in
    // luastatus.plugin) that still refer to w->data, may still be called by widget in event().
    // So do nothing.

    return NULL;
}

lua_State *
ew_call_begin(LS_ATTR_UNUSED_ARG void *userdata, size_t widget_idx)
{
    assert(widget_idx < nwidgets);

    Widget *w = &widgets[widget_idx]; // w->state can be anything here!

    LOCK_L_OR_S(w);
    lua_State *L = widget_event_lua_state(w);

    assert(lua_gettop(L) == 0); // L: -
    lua_rawgeti(L, LUA_REGISTRYINDEX, w->lua_ref_event); // L: event
    return L;
}

void
ew_call_end(LS_ATTR_UNUSED_ARG void *userdata, size_t widget_idx)
{
    assert(widget_idx < nwidgets);

    Widget *w = &widgets[widget_idx]; // w->state can be anything here!
    lua_State *L = widget_event_lua_state(w);

    assert(lua_gettop(L) == 2); // L: event arg

    if (w->lua_ref_event == LUA_REFNIL) {
        lua_pop(L, 2); // L: -
    } else {
        if (!check_lua_call(L, lua_pcall(L, 1, 0, 0))) {
            // L: -
            LOCK_B();
            set_error_unlocked(widget_idx);
            UNLOCK_B();
        } // else: L: -
    }

    UNLOCK_L_OR_S(w);
}

bool
prepare_stdio(void)
{
    // we rely on than stderr in unbuffered
    if (setvbuf(stderr, NULL, _IONBF, 0) != 0) {
        fprintf(stderr, "luastatus: (prepare) WARNING: setvbuf on stderr failed\n");
        fflush(stderr); // (!)
    }
    // we also rely a lot on *printf functions
    if (setlocale(LC_NUMERIC, "C") == NULL) {
        fprintf(stderr, "luastatus: (prepare) FATAL: setlocale failed\n");
        return false;
    }
    return true;
}

void
ignore_signal(LS_ATTR_UNUSED_ARG int signo)
{
}

bool
prepare_signals(void)
{
    // see ../DOCS/eintr-policy.md

    struct sigaction sa = {.sa_flags = SA_RESTART};
    if (sigemptyset(&sa.sa_mask) < 0) {
        LS_WITH_ERRSTR(s, errno,
            fprintf(stderr, "luastatus: (prepare) FATAL: sigemptyset: %s", s);
        );
        return false;
    }

#define HANDLE(SigNo_) \
    do { \
        if (sigaction(SigNo_, &sa, NULL) < 0) { \
            LS_WITH_ERRSTR(s, errno, \
                fprintf(stderr, "luastatus: (prepare) WARNING: sigaction (%s): %s", #SigNo_, s); \
            ); \
        } \
    } while (0)

    // we don't want to terminate on write to a dead pipe (especially if it's a barlib/plugin that
    // does that)
    // not SIG_IGN because children inherit the set of ignored signals
    sa.sa_handler = ignore_signal;
    HANDLE(SIGPIPE);

    // the sole purpose of this is to ensure SA_RESTART
    sa.sa_handler = SIG_DFL;
    // Another reason for not using SIG_IGN here is:
    //
    // http://pubs.opengroup.org/onlinepubs/009695399/functions/xsh_chap02_04.html#tag_02_04_03
    //
    // > If the action for the SIGCHLD signal is set to SIG_IGN, child processes of the calling
    // > processes shall not be transformed into zombie processes when they terminate. If the
    // > calling process subsequently waits for its children, and the process has no unwaited-for
    // > children that were transformed into zombie processes, it shall block until all of its
    // > children terminate, and wait(), waitid(), and waitpid() shall fail and set errno to ECHILD.
    HANDLE(SIGCHLD);
    // I have no idea what SIGURG is, but default action for it is to ignore, so let's ensure
    // SA_RESTART for it too.
    HANDLE(SIGURG);

    // we will also ensure SA_RESTART for any platform-specific signal that will annoy us.

#undef HANDLE
    return true;
}

bool
prepare(void)
{
    return prepare_stdio() && prepare_signals();
}

void
print_usage(void)
{
    fprintf(stderr, "USAGE: luastatus -b barlib [-B barlib_option [-B ...]] [-l loglevel] [-e] "
                    "widget.lua [widget2.lua ...]\n");
    fprintf(stderr, "       luastatus -v\n");
    fprintf(stderr, "See luastatus(1) for more information.\n");
}

void
print_version(void)
{
    fprintf(stderr, "This is luastatus %s.\n", LUASTATUS_VERSION);
}

bool
init_or_make_dummy(Widget *w)
{
    assert(w->state == WIDGET_STATE_LOADED);

    if (widget_init(w, (LuastatusPluginData) {.userdata = w, .logf = external_logf})) {
        lualibs_register_funcs(w, &barlib);
        return true;
    } else {
        internal_logf(LUASTATUS_ERR, "can't init widget '%s'", w->filename);
        widget_unload(w);
        widget_load_dummy(w);
        return false;
    }
}

int
main(int argc, char **argv)
{
    int ret = EXIT_FAILURE;
    char *barlib_name = NULL;
    LS_VECTOR_OF(char *) barlib_args = LS_VECTOR_NEW();
    bool no_hang = false;
    LS_VECTOR_OF(pthread_t) threads = LS_VECTOR_NEW();
    bool barlib_loaded = false;
    nwidgets = 0;
    widgets = NULL;

    for (int c; (c = getopt(argc, argv, "b:B:l:ev")) != -1;) {
        switch (c) {
        case 'b':
            barlib_name = optarg;
            break;
        case 'B':
            LS_VECTOR_PUSH(barlib_args, optarg);
            break;
        case 'l':
            if ((global_loglevel = loglevel_fromstr(optarg)) == LUASTATUS_LOGLEVEL_LAST) {
                fprintf(stderr, "Unknown loglevel '%s'.\n", optarg);
                print_usage();
                goto cleanup;
            }
            break;
        case 'e':
            no_hang = true;
            break;
        case 'v':
            print_version();
            goto cleanup;
        case '?':
            print_usage();
            goto cleanup;
        default:
            LS_UNREACHABLE();
        }
    }

    if (!prepare()) {
        goto cleanup;
    }

    if (!barlib_name) {
        internal_logf(LUASTATUS_FATAL, "barlib not specified");
        print_usage();
        goto cleanup;
    }

    nwidgets = argc - optind;
    widgets = LS_XNEW(Widget, nwidgets);
    for (size_t i = 0; i < nwidgets; ++i) {
        Widget *w = &widgets[i];
        const char *filename = argv[optind + i];
        if (!widget_load(w, filename)) {
            internal_logf(LUASTATUS_ERR, "can't load widget '%s'", filename);
            widget_load_dummy(w);
        }
    }

    if (!nwidgets) {
        internal_logf(LUASTATUS_WARN, "no widgets specified (see luastatus(1) for usage info)");
    }

    if (!load_barlib_by_name(&barlib, barlib_name)) {
        internal_logf(LUASTATUS_FATAL, "can't load barlib '%s'", barlib_name);
        goto cleanup;
    }
    barlib_loaded = true;

    if (!check_taints(&barlib, widgets, nwidgets)) {
        internal_logf(LUASTATUS_FATAL, "entities that share the same taint have been found");
        goto cleanup;
    }

    LS_VECTOR_PUSH(barlib_args, NULL);
    if (!barlib_init(&barlib, (LuastatusBarlibData) {.userdata = NULL, .logf = external_logf},
                     (const char *const *) barlib_args.data, nwidgets))
    {
        internal_logf(LUASTATUS_FATAL, "can't init the barlib");
        goto cleanup;
    }

    LS_VECTOR_RESERVE(threads, nwidgets);
    for (size_t i = 0; i < nwidgets; ++i) {
        Widget *w = &widgets[i];
        if (w->state != WIDGET_STATE_DUMMY && init_or_make_dummy(w)) {
            pthread_t t;
            PTH_CHECK(pthread_create(&t, NULL, widget_thread, w));
            LS_VECTOR_PUSH(threads, t);
        } else {
            LOCK_B();
            set_error_unlocked(i);
            UNLOCK_B();
        }
    }

    if (barlib.iface.event_watcher) {
        switch (barlib.iface.event_watcher(&barlib.data, ew_call_begin, ew_call_end)) {
        case LUASTATUS_BARLIB_EW_RESULT_NO_MORE_EVENTS:
            break;
        case LUASTATUS_BARLIB_EW_RESULT_FATAL_ERR:
            internal_logf(LUASTATUS_FATAL, "barlib's event_watcher() reported fatal error");
            fatal_error_reported();
            break;
        }
    }

    for (size_t i = 0; i < threads.size; ++i) {
        PTH_CHECK(pthread_join(threads.data[i], NULL));
    }

    internal_logf(LUASTATUS_WARN, "all plugins' run() and barlib's event_watcher() have returned");
    if (no_hang) {
        internal_logf(LUASTATUS_INFO, "-e passed, exiting");
        ret = EXIT_SUCCESS;
    } else {
        internal_logf(LUASTATUS_INFO, "since -e not passed, will hang now");
        while (1) {
            pause();
        }
    }

cleanup:
    LS_VECTOR_FREE(barlib_args);
    LS_VECTOR_FREE(threads);
    for (size_t i = 0; i < nwidgets; ++i) {
        widget_unload(&widgets[i]);
    }
    free(widgets);
    if (barlib_loaded) {
        barlib_unload(&barlib);
    }
    return ret;
}
