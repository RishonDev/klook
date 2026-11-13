#!/bin/bash
# SPDX-FileCopyrightText: 2026 Rishon Jonathan R
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Builds klook and dolphin-klook in an Arch Linux container (run as root from
# the repository root), then installs them and runs the smoke test against the
# installed klook:
#   packaging/arch/build-packages.sh <output dir>
# With REPO_GPG_KEY set (an armoured secret key), packages are signed.
set -euo pipefail
out=$(readlink -f "${1:?usage: $0 <output dir>}")
root=$(git rev-parse --show-toplevel)
mkdir -p "$out"

# makepkg refuses to run as root.
id builder > /dev/null 2>&1 || useradd -m builder
work=/home/builder/klook
rm -rf "$work"
cp -a "$root" "$work"
chown -R builder: "$work"

cd "$work/packaging/arch"
# Install everything both packages need to build (except klook itself).
mapfile -t deps < <(sudo -u builder makepkg --printsrcinfo |
    sed -n 's/^\s*\(make\)\?depends = //p' | sed 's/[<>=].*//' | grep -vx klook | sort -u)
pacman -S --noconfirm --needed "${deps[@]}"

# The keys KDE signs Dolphin releases with, for makepkg's signature check.
mapfile -t keys < <(sed -n 's/.*\b\([0-9A-F]\{40\}\)\b.*/\1/p' PKGBUILD)
sudo -u builder gpg --batch --keyserver hkps://keyserver.ubuntu.com --recv-keys "${keys[@]}"

sign=()
if [[ -n ${REPO_GPG_KEY:-} ]]; then
    sudo -u builder gpg --batch --import <<< "$REPO_GPG_KEY"
    keyid=$(sudo -u builder gpg --batch --list-secret-keys --with-colons | awk -F: '/^fpr/{print $10; exit}')
    sign=(--sign --key "$keyid")
fi

sudo -u builder git config --global --add safe.directory "$work"
sudo -u builder makepkg -f --noconfirm "${sign[@]}"
cp ./*.pkg.tar.zst* "$out/"

# Install what we built and test the packaged klook, not the build tree.
pacman -U --noconfirm --ask=4 ./*.pkg.tar.zst # --ask=4: replace the conflicting dolphin
pacman -S --noconfirm --needed dbus glib2 qt6-svg qt6-webengine
sudo -u builder dbus-run-session -- "$work/tests/smoke.sh" /usr/bin/klook
