# It now passes under helgrind, but with suppressions file (see ../../glib.supp).
#if [[ $PT_TOOL == helgrind ]]; then
#    return $PT_SKIP_ME_YES
#fi

# It does not pass under TSAN. Because glib is crazy or something like that.
if pt_cmake_opt_enabled WITH_TSAN; then
    return $PT_SKIP_ME_YES
fi

return $PT_SKIP_ME_NO
