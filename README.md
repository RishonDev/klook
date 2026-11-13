# klook

Quick Look for KDE Plasma. Select a file in Dolphin, press **Space**, and get an
instant preview; press **Space** again to close it. While it's open, moving the
selection in Dolphin moves the preview with it, just like Finder on macOS.

| Previews | |
|---|---|
| Images | PNG, JPEG, WebP, SVG, …; GIF/WebP animate |
| Code & text | Syntax highlighting (the engine Kate uses), follows your colour scheme |
| Markdown | Rendered, with relative images and clickable links |
| PDF | All pages, fitted to the window |
| Audio & video | Plays right away, with play/pause and seeking |
| Archives | zip/jar, tar.gz/xz/zst/bz2, 7z: file tree with sizes |
| Everything else | Icon, name, type and size |

## Install (Arch / CachyOS)

```sh
cd packaging/arch
makepkg -si
```

This builds two packages:

- **klook**: the previewer. It starts in the background at login so the first
  preview is instant too (about 30 MB of RAM).
- **dolphin-klook**: Dolphin with a *Quick Look* action on Space. It replaces
  the `dolphin` package; rebuild it when Dolphin updates. The patch lives in
  `dolphin/patches/`, and `dolphin/apply.sh` picks the newest one that fits.

Dolphin's own Space shortcut (selection mode) moves to the toolbar's *Select*
button; give it another key in *Settings → Configure Keyboard Shortcuts* if you
use it.

## Keys

| In Dolphin | |
|---|---|
| Space | Open / close the preview of the selection |
| Arrow keys | Move the selection; the preview follows |
| Esc | Close the preview |

| In a klook window (opened from a terminal or the right-click menu) | |
|---|---|
| Space / Esc | Close |
| ← / → | Previous / next file in the folder |
| Enter | Open with the default app |

## Command line

```sh
klook FILE           # preview FILE (again on the same file closes it)
klook --next         # step through the folder of the open preview
klook --previous
klook --no-activate FILE   # preview without taking keyboard focus
klook --daemon       # start in the background
klook --quit         # stop the background instance
```

Without the patched Dolphin, right-click any file → **Quick Look**.

## How it works

A single resident instance (`org.kde.klook` on the session bus) keeps the QML
engine warm, so a preview opens in well under 200 ms; running `klook` again just
hands it the file over D-Bus. File managers can drive it through
`org.kde.klook.Preview` at `/Preview`:

```
Show(s path, b activate)   Close()   IsShowing() → b   signal ShowingChanged(b)
```

With `activate = false` the preview is a layer-shell surface that never takes
keyboard focus. That's the only reliable way to do that on Wayland, and it's
what lets Dolphin keep handling the arrow keys.

## Build from source

```sh
cmake -B build -DCMAKE_INSTALL_PREFIX=~/.local [-DKLOOK_AUTOSTART=ON]
cmake --build build && cmake --install build
```

Needs Qt 6.8+ (Quick, DBus), KDE Frameworks 6 (WindowSystem, SyntaxHighlighting,
Archive, DBusAddons) and layer-shell-qt. Optional at runtime: QtMultimedia
(audio/video), QtPdf from qt6-webengine (PDF), qt6-svg.

Debug logging: `QT_LOGGING_RULES="klook.debug=true" QT_FORCE_STDERR_LOGGING=1 klook …`

## License

GPL-2.0-or-later
