#!/usr/bin/env bash
# Installs the Linux build for the current user (no sudo):
#   VST3        -> ~/.vst3/Headroom.vst3   (Reaper, Bitwig, Ardour, Carla scan this)
#   Standalone  -> ~/.local/bin/headroom (launches through `pw-jack` when PipeWire's JACK
#                  support is installed, so the app's JACK option goes through PipeWire)
#   App menu    -> ~/.local/share/applications/headroom.desktop
# Run it again after rebuilding to update. Remove with: scripts/install-linux.sh --uninstall
set -euo pipefail
cd "$(dirname "$0")/.."

vst3="$HOME/.vst3/Headroom.vst3"
bin="$HOME/.local/bin/headroom"
libdir="$HOME/.local/lib/headroom"
desktop="$HOME/.local/share/applications/headroom.desktop"
# The install from when the amp was called Evil Amplifier. Its plugin has the same ID, so a
# DAW would see two of it.
legacy=("$HOME/.vst3/Evil Amplifier.vst3" "$HOME/.local/bin/evil-amplifier"
        "$HOME/.local/lib/evil-amplifier" "$HOME/.local/share/applications/evil-amplifier.desktop")

if [ "${1:-}" = "--uninstall" ]; then
  rm -rf "$vst3" "$bin" "$libdir" "$desktop" "${legacy[@]}"
  echo "Removed Headroom."
  exit 0
fi

art=build/Headroom_artefacts/Release
if [ ! -d "$art/VST3/Headroom.vst3" ] || [ ! -x "$art/Standalone/Headroom" ]; then
  echo "No build found. Build first:  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build" >&2
  exit 1
fi

mkdir -p "$HOME/.vst3" "$HOME/.local/bin" "$libdir" "$HOME/.local/share/applications"
rm -rf "$vst3" "${legacy[@]}"
cp -r "$art/VST3/Headroom.vst3" "$vst3"
install -m 755 "$art/Standalone/Headroom" "$libdir/headroom"

# Wrapper: run through pw-jack if available, asking PipeWire for 128-sample buffers at 48 kHz
# (override with PIPEWIRE_QUANTUM=256/48000 headroom).
cat > "$bin" <<WRAPPER
#!/bin/sh
app="$libdir/headroom"
if command -v pw-jack >/dev/null 2>&1; then
  export PIPEWIRE_QUANTUM="\${PIPEWIRE_QUANTUM:-128/48000}"
  exec pw-jack "\$app" "\$@"
fi
exec "\$app" "\$@"
WRAPPER
chmod 755 "$bin"
cat > "$desktop" <<DESKTOP
[Desktop Entry]
Type=Application
Name=Headroom
Comment=Neural guitar amp simulator
Exec=$bin
Icon=audio-card
Terminal=false
Categories=AudioVideo;Audio;Music;
DESKTOP

echo "Installed:"
echo "  VST3        $vst3"
echo "  Standalone  $bin   (also in your app menu as 'Headroom')"
