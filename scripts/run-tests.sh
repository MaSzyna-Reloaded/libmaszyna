#!/usr/bin/env bash
# Run GUT test scripts in parallel and print one line per script.
#
# Why this exists rather than a hand-written loop each time:
#   * a script that fails to PARSE is not reported as a failing test - GUT ignores it, finds no
#     -gselect match and then runs the whole directory until the timeout, which reads exactly like
#     a hang. So every script is parse-checked first, cheaply, and a broken one is reported as
#     PARSE instead of costing a full timeout.
#   * running them one after another pays engine startup 40 times over; they fan out instead.
#   * the output is a short table, and the logs stay on disk to be read afterwards.
#
# Usage:
#   scripts/run-tests.sh <name> [<name>...]   run these test scripts (with or without .gd)
#   scripts/run-tests.sh --changed            run every test script changed against HEAD
#   JOBS=6 TIMEOUT=200 scripts/run-tests.sh --changed
#
# Exit status is the number of scripts that did not come back fully green.

set -uo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# This project runs on a double-precision Godot - world coordinates are kilometres from the
# scenery origin, so a single-precision binary gives different numbers instead of failing.
GODOT="${GODOT:-godot-double}"
command -v "$GODOT" >/dev/null 2>&1 || {
    echo "$GODOT not found - this project needs a double-precision Godot build" >&2
    exit 1
}
JOBS="${JOBS:-4}"
TIMEOUT="${TIMEOUT:-150}"
LOG_DIR="${LOG_DIR:-${TMPDIR:-/tmp}/maszyna-tests}"

mkdir -p "$LOG_DIR"

if [ "${1:-}" = "--changed" ]; then
    # only the test scripts themselves: fixtures carry no tests and would report NO RESULT
    mapfile -t NAMES < <(git -C "$REPO" diff --name-only HEAD -- demo/tests \
        | grep '^demo/tests/test_[^/]*\.gd$' | xargs -r -n1 basename | sed 's/\.gd$//' | sort -u)
else
    NAMES=("${@%.gd}")
fi

if [ "${#NAMES[@]}" -eq 0 ]; then
    echo "no test scripts to run"
    exit 0
fi

# Parse gate. A script that does not compile never reaches GUT.
# "Identifier not found: <Autoload>" is expected here and is not a parse error: --check-only -s
# starts a SceneTree without autoloads.
BROKEN=()
RUNNABLE=()
for name in "${NAMES[@]}"; do
    out="$($GODOT --headless --path "$REPO/demo" --check-only -s "res://tests/$name.gd" 2>&1)"
    if grep -q 'Parse Error' <<<"$out"; then
        printf '%-52s PARSE  %s\n' "$name" \
            "$(grep -m1 'Parse Error' <<<"$out" | sed 's/.*Parse Error: //')"
        BROKEN+=("$name")
    else
        RUNNABLE+=("$name")
    fi
done

# Each worker reports its own script as soon as it finishes, so a long batch shows progress
# instead of going silent until the end.
run_one() {
    name="$1"
    log="$LOG_DIR/$name.log"
    timeout "$TIMEOUT" "$GODOT" --headless --path "$REPO/demo" -s addons/gut/gut_cmdln.gd \
        -gdir=res://tests/ -gselect="$name" -gexit > "$log" 2>&1
    pass="$(grep -oP '^Passing Tests\s+\K\S+' "$log")"
    fail="$(grep -oP '^Failing Tests\s+\K\S+' "$log")"
    note=""
    grep -q 'dumped core' "$log" && note="CRASH"
    [ -z "$pass" ] && note="${note:-NO RESULT}"
    printf '%-52s pass=%-6s fail=%-6s %s\n' "$name" "${pass:-none}" "${fail:-0}" "$note"
    [ -z "$note" ] && [ -z "$fail" ]
}
export -f run_one
export LOG_DIR TIMEOUT GODOT REPO

printf '%s\n' "${RUNNABLE[@]}" | xargs -P "$JOBS" -I{} bash -c 'run_one "$@"' _ {}
GREEN=$?

FAILED=${#BROKEN[@]}
for name in "${RUNNABLE[@]}"; do
    log="$LOG_DIR/$name.log"
    grep -q '^Passing Tests' "$log" || { FAILED=$((FAILED + 1)); continue; }
    grep -q '^Failing Tests' "$log" && FAILED=$((FAILED + 1))
done

echo "--- $((${#RUNNABLE[@]} + ${#BROKEN[@]})) scripts, $FAILED not green, logs in $LOG_DIR"
exit "$FAILED"
