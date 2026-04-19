#!/usr/bin/env bash

set -e

my_dir=${1?}
dst_dir=${2?}

for f in changelog compat copyright; do
    cat "$my_dir"/templates_debian/"$f" > "$dst_dir"/"$f"
done

IS_LUAJIT=1
LUA_VER=5.1
cfg_files=( "$my_dir"/cfg/*.cfg )

gen_control() {
    local lua_pkg_suffix
    if (( IS_LUAJIT )); then
        lua_pkg_suffix="jit-$LUA_VER"
    else
        lua_pkg_suffix="$LUA_VER"
    fi

    while IFS= read -r line; do
        if [[ $line == '###{__build_deps__}###' ]]; then
            ./gen_debian_builddeps.py --lua-pkg-suffix="$lua_pkg_suffix" "${cfg_files[@]}" \
                || return $?
        else
            printf '%s\n' "$line"
        fi
    done < "$my_dir"/templates_debian/control

    ./gen_debian_control.py --lua-ver="$LUA_VER" "${cfg_files[@]}" \
        || return $?
}

gen_rules() {
    local lua_lib
    if (( IS_LUAJIT )); then
        lua_lib=luajit
    else
        lua_lib=lua"$LUA_VER"
    fi

    "$my_dir"/gen_debian_rules.py \
        --lua-lib="$lua_lib" \
        --input-file="$my_dir"/templates_debian/rules \
        "${cfg_files[@]}" \
        || return $?
}

gen_control > "$dst_dir"/control

gen_rules > "$dst_dir"/rules
