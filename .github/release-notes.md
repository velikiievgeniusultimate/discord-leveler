Tiny On/Off tray switch and a separate settings app for Linux KDE.

- Adaptive speech loudness leveling, linked stereo gain, noise threshold and peak limiter.
- Strict native Discord playback selection; browser/game/microphone routes stay unchanged.
- Live controls and input/output meters.
- Original Discord output restored on Off, shutdown and service crash cleanup.
- Autostart with the graphical session.
- Source installer builds for the local Qt version and runs DSP/isolation tests.

Install:

```bash
curl -fsSL https://raw.githubusercontent.com/velikiievgeniusultimate/discord-leveler/main/install.sh | bash
```

Update: `~/.local/bin/discord-leveler update`

Requires a Linux graphical session and PipeWire/PulseAudio. Native Discord only; browser Discord and ambiguous Flatpak process IDs are intentionally excluded. Simultaneous participants arrive as mixed audio and cannot be individually leveled. Read the README for dependency and latency details.
