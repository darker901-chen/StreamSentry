# StreamSentry

**Accident insurance for screen capture.** A Windows OBS Studio video
filter that deterministically masks the classic on-stream privacy
accidents *before* they reach your capture — and blacks the whole output
if it can't be sure it's working.

> Status: **v0.1, pre-release.** The detection and masking pipeline is
> built and machine-tested; several real-device checks are still a manual
> pass (see [Limitations](#limitations) and `reports/HUMAN_CHECKLIST.md`).

## The problem

You're live. Three things can leak private information in the half-second
before you notice:

1. **A notification toast** slides in with the content of a private
   message (LINE, Slack, Teams, Outlook…).
2. **A sensitive window** you didn't mean to show — a password manager, a
   credential dialog — is on the captured screen.
3. **A password field** reveals its contents via a "show password"
   toggle, an autofill dropdown, or a legacy plaintext app.

StreamSentry watches for these at the OS level and covers them with an
opaque plate, or — if it can't verify it's healthy — blacks the entire
source.

## How it works, and why

**Deterministic, never AI.** Detection is Win32 window enumeration plus
UI Automation — window class/process signatures and the UIA password
property. There is no machine learning anywhere in this plugin. A privacy
guarantee you can't audit isn't a guarantee; every masking decision here
is traceable to an explicit OS-level rule.

**Fail-closed is the product.** A single watcher thread owns detection and
updates a heartbeat every tick. If the heartbeat goes stale (>500 ms), if
coordinate mapping is uncertain, if the watcher thread dies, or if the
capture geometry can't be resolved — the **entire filter output goes
black** with a status banner. It starts black on load and only clears once
detection is confirmed alive. This behavior **cannot be turned off**;
there is no setting for it. Failing open would defeat the entire purpose.

**Opaque, never blur.** Masks are solid, opaque plates — a notification
card for toasts, a dark privacy plate (lock icon) for windows and password
fields. StreamSentry **never blurs or pixelates**, because blur and mosaic
are *reversible*: an archived clip can be attacked offline with deblurring
tools. Opaque is the only honest guarantee. Raw black is reserved solely
for the fail-closed state.

## Requirements

- Windows 10 21H2+ or Windows 11, x64
- OBS Studio 30 or newer

## Install

1. Download `streamsentry-0.1.0-windows-x64.zip` from the Releases page.
2. Extract it into your OBS Studio folder so that `streamsentry.dll` lands
   in `obs-plugins\64bit\` and the `data\obs-plugins\streamsentry\` folder
   is alongside OBS's other plugin data. (The zip mirrors OBS's layout:
   `streamsentry/bin/64bit/…` and `streamsentry/data/…`.)
3. Restart OBS. Confirm `obs-studio\...\logs` shows
   `[streamsentry] plugin loaded successfully`.

A self-built DLL may trigger a Windows SmartScreen warning; that is normal
for locally-built, unsigned binaries.

## Usage

1. Add a **Display Capture** source (this is the source type v0.1
   supports for coordinate mapping — see Limitations).
2. Right-click it → **Filters** → under **Effect Filters** add
   **StreamSentry**.
3. Settings:
   - **Enable** — turns the whole filter on/off.
   - **Blocklist** — one process name or window-title substring per line
     (case-insensitive). Matching windows are covered with a privacy
     plate. Pre-filled with sensible defaults (password managers and
     credential dialogs). Leave a line empty / clear the box to fall back
     to the built-in defaults.
   - There is deliberately **no** option to disable the fail-closed
     blackout.

Toast masking and password-field masking are always on while the filter
is enabled; they need no configuration.

## Limitations

StreamSentry is honest about what it does not do. In v0.1:

- **Display Capture only, unscaled, full-monitor.** Coordinate mapping in
  v0.1 resolves geometry only for an unscaled full-monitor Display
  Capture. A **Window Capture**, a scaled/cropped capture, or two monitors
  of *identical resolution* cannot be resolved with certainty, so the
  filter **fails closed** (black) rather than risk placing a mask in the
  wrong spot.
- **Toast over-masking.** On Windows 11 the toast host window class is
  shared with other XAML popups (Start-menu search, taskbar flyouts), so
  those may also get a notification card. This is deliberate — over-mask,
  never under-mask.
- **UAC / credential prompts on the secure desktop** are not capturable by
  OBS at all, so they cannot appear in your stream to begin with (the
  credential broker on the normal desktop *is* covered by the blocklist).
- **Custom-drawn UIs** (some Electron apps, games) may not expose the UIA
  password property; cover those with the blocklist instead.
- **Chromium browsers** enable their accessibility tree on demand; verify
  password-field masking per browser.
- **Fullscreen-exclusive games** are unsupported.
- **A 1–2 frame theoretical exposure** exists between a focus event and
  the mask being applied — milliseconds, but not zero.
- **IME candidate windows** (CJK input) are not handled.
- **RDP / virtual-machine environments** are unsupported.
- **Content-level secret detection** (spotting an API key or `.env` on
  screen) is impossible to do deterministically and is **permanently out
  of scope** — StreamSentry masks *windows and fields*, not arbitrary
  text.
- **UI is English-only** (localization is out of scope for v0.1).

Performance measured on the reference machine: watcher thread ~0.55% of
one core, added render cost ~0.03 µs/frame. A 30-minute soak showed a
stable working set; the SPEC's 2-hour endurance target is a manual check.

## Roadmap (not in v0.1)

Deliberately deferred: **allowlist mode** (only approved windows visible —
the planned v0.2 headline), Focus Assist / Do-Not-Disturb auto-integration
(v0.2), window-capture geometry, macOS support, per-app policies. Never
planned: blur/mosaic options, AI detection, content secret scanning, a
tray icon, auto-update.

## Building from source

Prerequisites: Visual Studio 2022 C++ build tools, CMake 3.28+, Git.

```
cmake --preset windows-x64
cmake --build --preset windows-x64
```

If configure fails because the preset pins a Windows SDK version your
machine doesn't have, create an untracked `CMakeUserPresets.json` with a
preset that inherits `windows-x64` but drops the SDK version pin
(`"architecture": "x64"` only), then build with that preset.

Pure-logic modules (coordinate mapping, plate generation) have unit tests:
`ctest --test-dir build_x64 -C RelWithDebInfo`.

## Documentation guide

**Using the plugin?** This README and [CHANGELOG.md](CHANGELOG.md) are
all you need (plus [LICENSE](LICENSE)).

**Reading or modifying the code?** Start with
[ARCHITECTURE.md](ARCHITECTURE.md) (how it is actually built),
[SPEC.md](SPEC.md) (what it must do, and what is deliberately out of
scope), and [TESTING.md](TESTING.md) (what has been verified, and how).

**Everything else is internal project process** — you can safely ignore
it: [CLAUDE.md](CLAUDE.md) (iron rules for the AI-assisted workflow that
builds this plugin), [SETUP.md](SETUP.md) (dev-box bootstrap notes,
Traditional Chinese, machine-specific), [reports/](reports/) (frozen
per-milestone verification evidence — see its own README),
`.claude/` (workflow agent definitions), `scripts/` (dev environment
bootstrap).

## License

GPL-2.0-or-later — see [LICENSE](LICENSE). All original code, written
against the OBS Studio and Windows SDK documentation.
