#!/bin/bash
# SPDX-FileCopyrightText: 2026 Rishon Jonathan R
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Adds Quick Look (Space) to Dolphin by applying the newest patch that fits
# without fuzz:
#   dolphin/apply.sh <dolphin source dir>
set -eu
here=$(dirname "$(readlink -f "$0")")
tree=${1:?usage: $0 <dolphin source dir>}

for patch in $(printf '%s\n' "$here"/patches/*.patch | sort -rV); do
    if patch -d "$tree" -p1 --dry-run --fuzz=0 -s -f < "$patch" > /dev/null 2>&1; then
        patch -d "$tree" -p1 --fuzz=0 -s < "$patch"
        echo "klook: applied dolphin/patches/$(basename "$patch")"
        exit 0
    fi
done
echo "klook: no patch in dolphin/patches fits this Dolphin" >&2
exit 1
