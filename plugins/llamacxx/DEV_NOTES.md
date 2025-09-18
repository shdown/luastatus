C is undeniably the best programming language out there, but there’s no limit to perfection.
Sometimes you start to wish there be the following features:
1. Constructors and destructors for structs, automatically generated from the list of members;
2. Defer-like chunks of code that get automatically run at the end of the scope (or at least at the end of the function).

Without these features, it is just too easy to overlook a struct member that needs to be destructed in the function that
destroys a struct, or to destruct a variable at the end of a function.

Point number 2, but not point number 1, can be done with GNU C’s `__attribute__(__cleanup__)` extension, but we limit
ourselves to standard C99 in this project.

This plugin is especially in the need to such checks, so, in order to deliver the best-possible software to our users,
we implemented a linter for C. It serves two purposes, matching the two nice-to-haves aboves:

1. Check that, in a struct, the sets of “heavy”✱ fields that are (1) declared, (2) initialized, and (3) freed, match each other.
2. Check that, in a function, the sets of “heavy”✱ local variables that are (1) declared/initialized at the beginning, and (2) freed in the end, match each other.

✱ “heavy” means requiring initialization and free-ing

The linter is implemented in Python and is contained in the `UTILS` subdirectory.
We call it “MLC” (for Memory Leak Checker).

Struct-scope checks look like this:

```c
//================================================================================
// Struct definition
//================================================================================

//MLC_PUSH_SCOPE("ConcQueue:decl")
typedef struct {
//MLC_DECL("mtx")
    pthread_mutex_t mtx;

//MLC_DECL("condvar")
    pthread_cond_t condvar;

//MLC_DECL("slots")
//MLC_DECL("slots[i]")
    LS_String *slots;
    size_t nslots;
    char slot_states[CONC_QUEUE_MAX_SLOTS];

    CONC_QUEUE_MASK new_data_mask;
} ConcQueue;
//MLC_POP_SCOPE()

//================================================================================
// Struct init function
//================================================================================

//MLC_PUSH_SCOPE("ConcQueue:init")
LS_INHEADER void conc_queue_create(ConcQueue *q, size_t nslots)
{
    assert(nslots <= CONC_QUEUE_MAX_SLOTS);

//MLC_INIT("mtx")
    LS_PTH_CHECK(pthread_mutex_init(&q->mtx, NULL));

//MLC_INIT("condvar")
    LS_PTH_CHECK(pthread_cond_init(&q->condvar, NULL));

//MLC_INIT("slots")
    q->slots = LS_XNEW(LS_String, nslots);

//MLC_INIT("slots[i]")
    for (size_t i = 0; i < nslots; ++i) {
        q->slots[i] = ls_string_new_reserve(1024);
        q->slot_states[i] = 'z';
    }
    q->nslots = nslots;

    q->new_data_mask = 0;
}
//MLC_POP_SCOPE()

//================================================================================
// Struct deinit function
//================================================================================

//MLC_PUSH_SCOPE("ConcQueue:deinit")
LS_INHEADER void conc_queue_destroy(ConcQueue *q)
{
//MLC_DEINIT("mtx")
    LS_PTH_CHECK(pthread_mutex_destroy(&q->mtx));

//MLC_DEINIT("condvar")
    LS_PTH_CHECK(pthread_cond_destroy(&q->condvar));

//MLC_DEINIT("slots[i]")
    for (size_t i = 0; i < q->nslots; ++i) {
        ls_string_free(q->slots[i]);
    }

//MLC_DEINIT("slots")
    free(q->slots);
}
//MLC_POP_SCOPE()
```

Function-scope checks look like this:

```c
//MLC_PUSH_SCOPE("run")
static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    size_t n = DSS_list_size(&p->dss_list);

//MLC_INIT("q")
    ConcQueue q;
    conc_queue_create(&q, n);

//MLC_INIT("slots")
//MLC_INIT("slots[i]")
    LS_String *slots = LS_XNEW(LS_String, n);
    for (size_t i = 0; i < n; ++i) {
        slots[i] = ls_string_new_reserve(1024);
    }
    char slot_states[CONC_QUEUE_MAX_SLOTS] = {0};

//MLC_INIT("answer_str")
    LS_String answer_str = ls_string_new_reserve(1024);
//MLC_INIT("prompt_str")
    LS_String prompt_str = ls_string_new_reserve(1024);

//MLC_INIT("R")
    Requester *R = NULL;

//MLC_INIT("tid_list[i]")
//MLC_INIT("tid_list")
    pthread_t *tid_list = LS_XNEW(pthread_t, n);
    size_t tid_list_n = 0;

//MLC_INIT("e")
    MyError e = {0};

    //==============================
    // ... some code in-between ...
    //==============================

fail:
//MLC_DEINIT("tid_list[i]")
    for (size_t i = 0; i < tid_list_n; ++i) {
        LS_PTH_CHECK(pthread_join(tid_list[i], NULL));
    }
//MLC_DEINIT("tid_list")
    free(tid_list);

//MLC_DEINIT("q")
    conc_queue_destroy(&q);

//MLC_DEINIT("slots[i]")
    for (size_t i = 0; i < n; ++i) {
        ls_string_free(slots[i]);
    }
//MLC_DEINIT("slots")
    free(slots);

//MLC_DEINIT("answer_str")
    ls_string_free(answer_str);
//MLC_DEINIT("prompt_str")
    ls_string_free(prompt_str);
//MLC_DEINIT("R")
    if (R) {
        requester_destroy(R);
    }
//MLC_DEINIT("e")
    my_error_dispose(&e);
}
//MLC_POP_SCOPE()
```

There is also support for branching (success path vs. failure path) in a function
(`MLC_MODE`), but none of the code currently uses it.
