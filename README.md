# Discord Leveler

A tiny Linux tray switch that levels voices from **native desktop Discord only**. Built for KDE Plasma, Qt 6 and PipeWire's PulseAudio compatibility server. No EasyEffects required.

Click the circular **On / Off** tray button to switch processing. Right-click → Settings, or open **Discord Leveler** from your application menu. KDE may initially put new tray icons in the overflow menu: set Discord Leveler to “Always shown” in System Tray settings.

## Install / update

```bash
curl -fsSL https://raw.githubusercontent.com/velikiievgeniusultimate/discord-leveler/main/install.sh | bash
```

The installer downloads the latest GitHub release, verifies its SHA256 checksum, builds against your system's Qt libraries, runs DSP tests and installs to `~/.local`. It installs missing build dependencies using your system package manager with normal administrator authorization. Supports Arch, Debian/Ubuntu and Fedora; other distributions need the documented build dependencies installed manually. Run it in a logged-in graphical session.

Update with the same install command, or:

```bash
~/.local/bin/discord-leveler update
```

Uninstall completely:

```bash
~/.local/bin/discord-leveler uninstall
```

## Controls

- Target loudness: -30 to -12 dBFS RMS (default -20).
- Maximum boost: 0 to 24 dB (default 12).
- Noise threshold: do not increase gain for background noise below this level (default -48 dBFS).
- Loud voice reaction: 5–200 ms (default 30).
- Quiet voice recovery: 100–2000 ms (default 700).
- Peak ceiling: -6 to -0.5 dBFS (default -1).

Changes apply live. Settings and meters are in a separate window; closing it leaves the tray switch running. Settings persist across updates. Autostart uses a user systemd service tied to the graphical session. Off removes the private sink and returns Discord to its original output. Service shutdown/crash cleanup restores routes and removes the sink.

## Isolation and limitations

Only playback streams whose `application.process.binary` is Discord/DiscordCanary/DiscordPTB **and whose live `/proc/PID/exe` matches** are routed. A generic `WEBRTC VoiceEngine` label is never sufficient. Browser Discord, Flatpak Discord with ambiguous namespace PIDs, Vesktop and other clients are intentionally unsupported in v0.1. No microphone/source routes, default devices or app volumes are changed. New Discord streams are discovered every 500 ms.

Discord mixes its participants before exposing audio to the OS. This levels people speaking in turn; it cannot separately level two overlapping speakers. All native Discord playback, including notifications, passes through the leveler while On. Stereo balance is preserved with linked gain. A 10 ms processing block plus audio-server buffers adds latency. This is an adaptive speech leveler with a limiter, not an LUFS mastering tool.

The audio engine captures only the private Discord sink, never the physical device's monitor. Never select the private sink as another application's output. It has filter class and zero session priority, and is never set as the default. The app uses local `pactl` and libpulse; it does not log in to Discord or inspect conversations.

## Build

C++17, CMake, pkg-config, Qt6 Widgets/Network/DBus, libpulse-simple and `pactl`.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/discord-leveler --settings
```

CLI: `--settings`, `--tray`, `--on`, `--off`, `--toggle`, `--quit`. D-Bus interface: `io.github.DiscordLeveler`, object `/io/github/DiscordLeveler`, methods `toggle`, `settings`, `setEnabled` and boolean property `enabled`.

PipeWire/PulseAudio may remember previous per-application routing; startup cleanup removes stale private sinks. Explicit user routing of Discord while On is overridden until Off. Output follows changes to the system default for the leveler's own stream only.

MIT license.
