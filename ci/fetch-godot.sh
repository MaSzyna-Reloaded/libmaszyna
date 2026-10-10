#!/usr/bin/env bash
# Fetch the double precision Godot built by libmaszyna's .github/workflows/godot-engine.yml instead
# of building it - from libmaszyna's release in any repository, a game's CI included. The files
# land in the Makefile's $(GODOT_BIN), exactly where its engine rules would put them, so nothing is
# rebuilt; the editor is installed as godot-double and the templates where the export looks them up.
#
# Usage: ci/fetch-godot.sh [<file>...]   only these files of the release (default: all of them)

set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="$(make -s -C "$REPO_DIR" godot-version)"
TAG="godot-$VERSION-double"
REPOSITORY="${GODOT_RELEASE_REPOSITORY:-MaSzyna-Reloaded/libmaszyna}"
GODOT_BIN="$REPO_DIR/build-godot-$VERSION/bin"
BIN_DIR="${BIN_DIR:-$HOME/.local/bin}"
TEMPLATES_DIR="$HOME/.local/share/godot/export_templates/$VERSION.stable.double"

# release file -> where Godot expects it; the editor goes into BIN_DIR
declare -A INSTALL=(
    [godot.linuxbsd.editor.double.x86_64]="$BIN_DIR/godot-double"
    [godot.linuxbsd.template_release.double.x86_64]="$TEMPLATES_DIR/linux_release.x86_64"
    [godot.linuxbsd.template_debug.double.x86_64]="$TEMPLATES_DIR/linux_debug.x86_64"
    [godot.windows.template_release.double.x86_64.exe]="$TEMPLATES_DIR/windows_release_x86_64.exe"
    [godot.windows.template_debug.double.x86_64.exe]="$TEMPLATES_DIR/windows_debug_x86_64.exe"
    [android_release.apk]="$TEMPLATES_DIR/android_release.apk"
    [android_debug.apk]="$TEMPLATES_DIR/android_debug.apk"
)

FILES=("$@")
[ "${#FILES[@]}" -eq 0 ] && FILES=("${!INSTALL[@]}")

mkdir -p "$GODOT_BIN"
for file in "${FILES[@]}"; do
    target="${INSTALL[$file]:?unknown file: $file}"
    if [ ! -f "$GODOT_BIN/$file" ]; then
        curl -fsSL -o "$GODOT_BIN/$file" \
            "https://github.com/$REPOSITORY/releases/download/$TAG/$file"
    fi
    install -D -m 755 "$GODOT_BIN/$file" "$target"
    echo "$file -> $target"
done
