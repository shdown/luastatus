The list of locking patterns of the main luastatus binary; run

    grep -Ew '(UN)?LOCK_[A-Z]' luastatus/luastatus.c

to verify it is up-to-date.

It can be said that our order is: E < L < B.
We also don't lock the same mutex twice in any of the “procedures”.
This suffices to say there are no deadlocks.

(We also have the `tests/torture.sh` test!)

    cb-gets-called() {
        lock L
        lock B
        unlock B
        unlock L
    }

    plugin-begins-call-and-cancels() {
        lock L
        unlock L
    }

    #-----------------------------------------------

    event-gets-called-and-raises-error-E() {
        lock E
        lock B
        unlock B
        unlock E
    }

    events-gets-called-and-succeeds-E() {
        lock E
        unlock E
    }

    barlib-ew-begins-call-and-cancels-E() {
        lock E
        unlock E
    }

    #-----------------------------------------------

    event-gets-called-and-raises-error-L() {
        lock L
        lock B
        unlock B
        unlock L
    }

    events-gets-called-and-succeeds-L() {
        lock L
        unlock L
    }

    barlib-ew-begins-call-and-cancels-L() {
        lock L
        unlock L
    }

    #-----------------------------------------------

    set-error-when-plugin-run-returned() {
        lock B
        unlock B
    }

    set-error-when-widget-init-failed() {
        lock B
        unlock B
    }
