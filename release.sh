#!/bin/sh
set -e

if [ "$#" -ne 1 ]; then
    echo >&2 "USAGE: $0 new_version"
    exit 2
fi

RELEASE_BRANCH=release
DEV_BRANCH=master
NEW_VERSION=$1

T() {
    printf >&2 -- '-> %s\n' "$*"
    "$@"
}

T_redir_to_file() {
    local f=$1
    shift
    printf >&2 -- '-> %s > %s\n' "$*" "$f"
    "$@" > "$f"
}

T_append_to_file() {
    local f=$1
    shift
    printf >&2 -- '-> %s >> %s\n' "$*" "$f"
    "$@" >> "$f"
}

ask() {
    local ans
    echo ""
    echo -n "  $* (“yes” to proceed) >>> "
    read -r ans || exit 3
    if [ "$ans" != yes ]; then
        exit 3
    fi
}

if ! T git checkout $RELEASE_BRANCH; then
    T git branch $RELEASE_BRANCH
    T git checkout $RELEASE_BRANCH
fi

last_msg=$(git log -1 --pretty=%B)
release_msg="Release $NEW_VERSION"
if [ "$last_msg" = "$release_msg" ]; then
    echo ""
    echo "Last commit message is “$last_msg”."
    echo "Apparently, you ran this script before, and “git tag”/“git push” had failed."
    ask 'Jump to the “git tag” part?'
else
    T git merge $DEV_BRANCH
    T_redir_to_file VERSION echo $NEW_VERSION
    T_append_to_file RELEASE_NOTES echo
    T sed \
        -e "1i\luastatus $NEW_VERSION ($(LC_ALL=C date +'%B %d, %Y'))\n===\n(Please describe the changes here.)\n" \
        -e :a -e '/^\n*$/{$d;N;};/\n$/ba' \
        -i RELEASE_NOTES
    T ${EDITOR:-edit} RELEASE_NOTES
    ask 'Commit and push?'
    T git add VERSION RELEASE_NOTES
    T git commit -m "$release_msg"
fi

T git tag --force v$NEW_VERSION
T git push --set-upstream origin $RELEASE_BRANCH
T git push --tags

if ! T git checkout $DEV_BRANCH; then
    echo ""
    echo "Ignoring this to show the message below."
fi

cat <<EOF

===
Done! Now:
  * Release it on GitHub:
    * The “<number> release(s)” link on the GitHub page of the project
    * The “Tags” tab
    * “Add release notes”
    * Fill in everything needed, click “Publish release”
  * Tell everybody about it!!
===
EOF
