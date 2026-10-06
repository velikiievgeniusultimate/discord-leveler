#!/usr/bin/env bash
set -euo pipefail
repo=velikiievgeniusultimate/discord-leveler
prefix="${HOME}/.local"
app_dir="${prefix}/share/discord-leveler"
mode="${1:-}"
if [[ "$mode" != "--local" ]]; then
    command -v curl >/dev/null || { echo 'Install curl first.' >&2; exit 1; }
    tmp_dir=$(mktemp -d)
    trap 'rm -rf "$tmp_dir"' EXIT
    base="https://github.com/${repo}/releases/latest/download"
    curl -fL --retry 3 "$base/discord-leveler-source.tar.gz" -o "$tmp_dir/discord-leveler-source.tar.gz"
    curl -fL --retry 3 "$base/SHA256SUMS" -o "$tmp_dir/SHA256SUMS"
    (cd "$tmp_dir" && sha256sum -c SHA256SUMS)
    tar -xzf "$tmp_dir/discord-leveler-source.tar.gz" -C "$tmp_dir"
    bash "$tmp_dir/discord-leveler/install.sh" --local
    exit
fi
source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
if ! command -v cmake >/dev/null || ! command -v g++ >/dev/null || ! command -v pkg-config >/dev/null || ! pkg-config --exists Qt6Widgets Qt6Network Qt6DBus libpulse-simple || ! command -v pactl >/dev/null; then
    echo 'Installing Qt 6, build tools and PulseAudio client libraries (PipeWire-compatible).'
    if command -v pacman >/dev/null; then
        if command -v pkexec >/dev/null; then
            pkexec pacman -S --needed cmake gcc make pkgconf qt6-base libpulse
        else
            sudo pacman -S --needed cmake gcc make pkgconf qt6-base libpulse
        fi
    elif command -v apt-get >/dev/null; then
        sudo apt-get update
        sudo apt-get install -y cmake g++ make pkg-config qt6-base-dev libpulse-dev pulseaudio-utils
    elif command -v dnf >/dev/null; then
        sudo dnf install -y cmake gcc-c++ make pkgconf-pkg-config qt6-qtbase-devel pulseaudio-libs-devel pulseaudio-utils
    else
        echo 'Please install cmake, a C++17 compiler, Qt6 base development libraries, libpulse development libraries, pkg-config and pactl.' >&2
        exit 1
    fi
fi
cmake -S "$source_dir" -B "$source_dir/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$source_dir/build" --parallel 2
ctest --test-dir "$source_dir/build" --output-on-failure
systemctl --user stop discord-leveler.service 2>/dev/null || true
systemctl --user stop discord-leveler-preview.service 2>/dev/null || true
mkdir -p "$app_dir" "$prefix/bin" "$prefix/share/applications" "$prefix/share/icons/hicolor/scalable/apps" "$HOME/.config/systemd/user"
install -m755 "$source_dir/build/discord-leveler" "$app_dir/discord-leveler"
install -m755 "$source_dir/scripts/update.sh" "$app_dir/update.sh"
install -m755 "$source_dir/scripts/uninstall.sh" "$app_dir/uninstall.sh"
install -m644 "$source_dir/scripts/icon.svg" "$prefix/share/icons/hicolor/scalable/apps/discord-leveler.svg"
cat > "$prefix/bin/discord-leveler" <<LAUNCH
#!/usr/bin/env bash
case "\${1:-}" in
  update) exec bash "$app_dir/update.sh" ;;
  uninstall) exec bash "$app_dir/uninstall.sh" ;;
  *) exec "$app_dir/discord-leveler" "\$@" ;;
esac
LAUNCH
chmod +x "$prefix/bin/discord-leveler"
cat > "$HOME/.config/systemd/user/discord-leveler.service" <<SERVICE
[Unit]
Description=Discord-only voice loudness leveler
After=graphical-session.target pipewire-pulse.service
PartOf=graphical-session.target

[Service]
Type=simple
ExecStart="$app_dir/discord-leveler" --tray
ExecStopPost="$app_dir/discord-leveler" --cleanup
Restart=on-failure
RestartSec=2
KillMode=control-group
TimeoutStopSec=5

[Install]
WantedBy=graphical-session.target
SERVICE
cat > "$prefix/share/applications/io.github.DiscordLeveler.desktop" <<DESKTOP
[Desktop Entry]
Type=Application
Name=Discord Leveler
Comment=Settings for Discord voice loudness leveling
Comment[ru]=Выравнивание громкости голосов Discord
Exec="$app_dir/discord-leveler" --settings
Icon=discord-leveler
Categories=AudioVideo;Audio;Settings;
Terminal=false
StartupNotify=false
DESKTOP
systemctl --user daemon-reload
systemctl --user enable --now discord-leveler.service
command -v update-desktop-database >/dev/null && update-desktop-database "$prefix/share/applications" || true
command -v kbuildsycoca6 >/dev/null && kbuildsycoca6 --noincremental >/dev/null 2>&1 || true
printf '\nInstalled. Settings: Discord Leveler in your application menu.\nUpdate: ~/.local/bin/discord-leveler update\n'
