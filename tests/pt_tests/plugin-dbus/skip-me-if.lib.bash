# It does not pass under TSAN and helgrind. Because glib is crazy or something like that.

if [[ $PT_TOOL == helgrind ]]; then
    return $PT_SKIP_ME_YES
fi

if pt_cmake_opt_enabled WITH_TSAN; then
    return $PT_SKIP_ME_YES
fi

return $PT_SKIP_ME_NO
