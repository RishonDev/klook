#!/bin/bash
# SPDX-FileCopyrightText: 2026 Rishon Jonathan R
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Moves dolphin-klook to the dolphin version in Arch's repositories. Run in an
# Arch container from the repository root:
#   packaging/arch/update-dolphin.sh
# Exit status: 0 = updated (see the summary on stdout), 3 = already current,
# 1 = failed; when the patch can't be carried over, the reason is written to
# packaging/arch/update-failure.md for the issue CI opens.
set -euo pipefail
root=$(git rev-parse --show-toplevel)
pkgbuild=$root/packaging/arch/PKGBUILD
failure=$root/packaging/arch/update-failure.md

current_ver=$(sed -n 's/^_dolphinver=//p' "$pkgbuild")
current_rel=$(sed -n 's/^_dolphinrel=//p' "$pkgbuild")
pacman -Sy --noconfirm > /dev/null
latest=$(pacman -Si dolphin | awk -F': ' '/^Version/{print $2; exit}')
new_ver=${latest%-*}
new_rel=${latest##*-}

if [[ $new_ver == "$current_ver" && $new_rel == "$current_rel" ]]; then
    echo "dolphin $latest: already current"
    exit 3
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
url=https://download.kde.org/stable/release-service/%s/src/dolphin-%s.tar.xz

if [[ $new_ver != "$current_ver" ]]; then
    printf -v new_url "$url" "$new_ver" "$new_ver"
    curl -sSfL --retry 4 --retry-all-errors -o "$work/new.tar.xz" "$new_url"
    sha=$(sha256sum "$work/new.tar.xz" | cut -d' ' -f1)
    mkdir "$work/new" && tar -xf "$work/new.tar.xz" -C "$work/new" --strip-components=1

    # Does an existing patch still apply cleanly? Try on a throwaway copy.
    cp -a "$work/new" "$work/try"
    # (KLOOK_FORCE_REBASE=1 skips this, to exercise the rebase path in tests.)
    if [[ -z ${KLOOK_FORCE_REBASE:-} ]] && "$root/dolphin/apply.sh" "$work/try" > /dev/null 2>&1; then
        note="the existing patch applies unchanged"
    else
        # Carry the patch over with a 3-way merge: old source + patch, rebased
        # onto the new source, exactly like rebasing a branch.
        printf -v old_url "$url" "$current_ver" "$current_ver"
        curl -sSfL --retry 4 --retry-all-errors -o "$work/old.tar.xz" "$old_url"
        repo=$work/repo
        mkdir "$repo" && tar -xf "$work/old.tar.xz" -C "$repo" --strip-components=1
        git -C "$repo" init -q
        git -C "$repo" -c user.name=ci -c user.email=ci@localhost add -A
        git -C "$repo" -c user.name=ci -c user.email=ci@localhost commit -qm "dolphin $current_ver"
        "$root/dolphin/apply.sh" "$repo"
        git -C "$repo" -c user.name=ci -c user.email=ci@localhost add -A
        git -C "$repo" -c user.name=ci -c user.email=ci@localhost commit -qm "klook patch"
        git -C "$repo" branch patched
        git -C "$repo" checkout -q -b new HEAD~1
        git -C "$repo" rm -rq .
        tar -xf "$work/new.tar.xz" -C "$repo" --strip-components=1
        git -C "$repo" -c user.name=ci -c user.email=ci@localhost add -A
        git -C "$repo" -c user.name=ci -c user.email=ci@localhost commit -qm "dolphin $new_ver"
        git -C "$repo" tag upstream-new
        if ! git -C "$repo" -c user.name=ci -c user.email=ci@localhost cherry-pick patched > "$work/pick.log" 2>&1; then
            {
                echo "Dolphin $new_ver is in Arch, but the Quick Look patch no longer applies and"
                echo "a 3-way rebase from $current_ver hit conflicts:"
                echo
                echo '```'
                git -C "$repo" diff --name-only --diff-filter=U
                echo
                cat "$work/pick.log"
                echo '```'
                echo
                echo "Rebase \`dolphin/patches/\` onto $new_ver by hand, then re-run the *Dolphin update* workflow."
            } > "$failure"
            echo "patch conflicts with dolphin $new_ver" >&2
            exit 1
        fi
        git -C "$repo" diff upstream-new HEAD -- src > "$root/dolphin/patches/$new_ver.patch"
        note="the patch was rebased onto $new_ver (dolphin/patches/$new_ver.patch)"
    fi

    sed -i "s/^_dolphinver=.*/_dolphinver=$new_ver/" "$pkgbuild"
    # The tarball checksum is the first entry; the signature's stays SKIP.
    sed -i "s/^sha256sums=('[0-9a-f]*'/sha256sums=('$sha'/" "$pkgbuild"
else
    note="same Dolphin release, rebuilt against Arch's new dolphin $latest"
fi
sed -i "s/^_dolphinrel=.*/_dolphinrel=$new_rel/" "$pkgbuild"

echo "Follow Arch's dolphin $latest"
echo
echo "Was $current_ver-$current_rel; $note."
