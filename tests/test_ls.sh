#!/usr/bin/env bash
# Behavioral smoke tests. GNU ls is used as a reference for shared options.
set -euo pipefail

PROGRAM="${1:-./bin/myls}"
PROGRAM="$(realpath "$PROGRAM")"
REFERENCE="$(command -v ls)"
export LC_ALL=C
WORKSPACE="$(mktemp -d)"
trap 'rm -rf "$WORKSPACE"' EXIT
mkdir -p "$WORKSPACE/fixture/dir/subdir" "$WORKSPACE/fixture/empty dir"
printf 'small\n' > "$WORKSPACE/fixture/alpha.txt"
printf 'This is a slightly larger sample file.\n' > "$WORKSPACE/fixture/zeta.txt"
printf 'hidden\n' > "$WORKSPACE/fixture/.secret"
printf '#!/bin/sh\necho hi\n' > "$WORKSPACE/fixture/run.sh"
printf 'inside\n' > "$WORKSPACE/fixture/dir/inside.txt"
printf 'nested\n' > "$WORKSPACE/fixture/dir/subdir/deep.txt"
chmod +x "$WORKSPACE/fixture/run.sh"
ln -s alpha.txt "$WORKSPACE/fixture/link-to-file"
ln -s dir "$WORKSPACE/fixture/link-to-dir"
ln -s not-here "$WORKSPACE/fixture/broken-link"
touch -t 202001010101 "$WORKSPACE/fixture/alpha.txt"
touch -t 202201010101 "$WORKSPACE/fixture/zeta.txt"

passed=0
compare() {
    local title="$1"
    shift
    # -1 ensures that both programs produce one filename per output line.
    "$REFERENCE" -1 "$@" > "$WORKSPACE/expected"
    "$PROGRAM" -1 "$@" > "$WORKSPACE/actual"
    if ! diff -u "$WORKSPACE/expected" "$WORKSPACE/actual"; then
        printf 'FAIL: %s\n' "$title" >&2
        exit 1
    fi
    printf 'PASS: %s\n' "$title"
    passed=$((passed + 1))
}

cd "$WORKSPACE/fixture"
compare 'default name sort' .
compare 'include hidden (and dot entries)' -a .
compare 'almost all' -A .
compare 'reverse name sort' -r .
compare 'mtime sort' -t .
compare 'size sort' -S .
compare 'sort by time reverse' -tr .
compare 'unsorted' -U .
compare 'directories as entries' -d . dir
compare 'inode values' -i .
compare 'file indicators' -F .
compare 'directory suffixes' -p .
compare 'directory recursion' -R dir
compare 'recursive including hidden' -aR dir
compare 'multiple mixed operands' alpha.txt dir zeta.txt
compare 'spaces and symlinks' 'empty dir' link-to-file
printf 'dash file\n' > ./-dash-file
compare 'explicit -- stops option processing' -- -dash-file

# Error cases must return a nonzero exit status without crashing.
if "$PROGRAM" does-not-exist > /dev/null 2> "$WORKSPACE/error"; then
    echo 'FAIL: missing operand returned success' >&2
    exit 1
fi
grep -q 'does-not-exist' "$WORKSPACE/error"
printf 'PASS: missing file error\n'
passed=$((passed + 1))

# A valid file is still displayed even when another operand is missing.
if "$PROGRAM" does-not-exist alpha.txt > "$WORKSPACE/actual" 2>/dev/null; then
    echo 'FAIL: mixed operands returned success' >&2
    exit 1
fi
grep -qx 'alpha.txt' "$WORKSPACE/actual"
printf 'PASS: partial success with nonzero exit\n'
passed=$((passed + 1))

# Check mode bits, symbolic-link destination, and numeric ownership.
"$PROGRAM" -l run.sh link-to-file > "$WORKSPACE/actual"
grep -q '^-rwx' "$WORKSPACE/actual"
grep -q 'link-to-file -> alpha.txt' "$WORKSPACE/actual"
"$PROGRAM" -n alpha.txt | grep -Eq '^[^ ]+ +[0-9]+ +[0-9]+ +[0-9]+'
printf 'PASS: long listings and symlink target\n'
passed=$((passed + 1))

"$PROGRAM" -lh alpha.txt > "$WORKSPACE/actual"
grep -q 'alpha.txt' "$WORKSPACE/actual"
"$PROGRAM" --help | grep -q 'Usage:'
printf 'PASS: human-readable and help\n'
passed=$((passed + 1))

printf '\nAll %d tests passed.\n' "$passed"
