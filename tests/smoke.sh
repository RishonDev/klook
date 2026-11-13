#!/bin/bash
# SPDX-FileCopyrightText: 2026 Rishon Jonathan R
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Opens every sample with klook offscreen and fails on QML errors or crashes,
# then exercises the D-Bus API and folder navigation:
#   tests/smoke.sh <path to klook binary>
# Needs a session bus; CI runs it under dbus-run-session.
set -u
klook=$(readlink -f "${1:?usage: $0 <klook binary>}")
samples=$(dirname "$(readlink -f "$0")")/samples
log=$(mktemp)
failures=0
export QT_QPA_PLATFORM=offscreen QT_FORCE_STDERR_LOGGING=1 QT_LOGGING_RULES="klook.debug=true"

pass() { echo "PASS $*"; }
fail() { echo "FAIL $*"; failures=$((failures + 1)); }

# A preview failed if QML reported a problem in klook's own files, or klook
# itself logged a failure. (Missing platform plugins etc. are expected offscreen.)
problems() { grep -E 'qrc:/qt/qml/klook/|klook: (failed|no such file)' "$1"; }

# klook stays resident, so `timeout` ending it (124) is success; anything else
# non-zero is a crash or a startup error.
for file in "$samples"/*; do
    name=${file##*/}
    timeout 4 "$klook" "$file" > "$log" 2>&1
    status=$?
    if [[ $status != 124 && $status != 0 ]]; then
        fail "$name: exit status $status"; cat "$log"
    elif problems "$log"; then
        fail "$name: QML/klook errors above"
    else
        pass "$name"
    fi
done

# --- Resident instance, D-Bus API and navigation ----------------------------
call() { gdbus call --session --dest org.kde.klook --object-path /Preview --method "org.kde.klook.Preview.$1" "${@:2}"; }

"$klook" --daemon > "$log" 2>&1 &
daemon=$!
for _ in $(seq 50); do gdbus introspect --session --dest org.kde.klook --object-path /Preview > /dev/null 2>&1 && break; sleep 0.1; done

call Show "'$samples/folder/a.txt'" false > /dev/null
sleep 0.5
[[ $(call IsShowing) == "(true,)" ]] && pass "D-Bus Show" || fail "D-Bus Show: $(call IsShowing)"

timeout 5 "$klook" --next
sleep 0.5
grep -q 'swapped ".*/folder/b.txt"' "$log" && pass "--next steps to b.txt" || fail "--next did not reach b.txt"

timeout 5 "$klook" --next # end of the folder: must stay on b.txt
sleep 0.3
[[ $(grep -c 'swapped' "$log") == 1 ]] && pass "--next stops at the end" || fail "--next wrapped or moved past the end"

call Close > /dev/null
sleep 0.5
[[ $(call IsShowing) == "(false,)" ]] && pass "D-Bus Close" || fail "D-Bus Close: $(call IsShowing)"

timeout 5 "$klook" --quit
if timeout 5 tail --pid=$daemon -f /dev/null; then pass "--quit"; else fail "--quit left the instance running"; kill $daemon; fi
problems "$log" && fail "resident instance logged QML/klook errors"

rm -f "$log"
echo "RESULT smoke: $([[ $failures == 0 ]] && echo PASS || echo "FAIL ($failures)")"
exit $((failures > 0))
