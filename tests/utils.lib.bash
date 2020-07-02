resolve_relative() {
    if [[ $1 == /* ]]; then
        printf '%s\n' "$1"
    else
        printf '%s\n' "$2/$1"
    fi
}
