#!/usr/bin/env bash
set -euo pipefail
prefix="$HOME/.local"
systemctl --user disable --now discord-leveler.service 2>/dev/null || true
"$prefix/share/discord-leveler/discord-leveler" --cleanup || true
rm -f "$HOME/.config/systemd/user/discord-leveler.service" "$prefix/bin/discord-leveler" "$prefix/share/applications/io.github.DiscordLeveler.desktop" "$prefix/share/icons/hicolor/scalable/apps/discord-leveler.svg"
rm -rf "$prefix/share/discord-leveler" "$HOME/.config/discord-leveler"
systemctl --user daemon-reload
command -v kbuildsycoca6 >/dev/null && kbuildsycoca6 --noincremental >/dev/null 2>&1 || true
echo 'Discord Leveler removed. Discord uses its direct output.'
