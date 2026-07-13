# StreamSentry

**A privacy assist for screen capture.** A Windows OBS Studio video
filter that deterministically masks the classic on-stream privacy
accidents *before* they reach your capture — and clearly tells you
(with an on-output status chip) whenever it cannot verify it is
protecting you. It never interrupts your stream.

> Status: **v0.2, pre-release.** Machine-verified (4 unit-test suites +
> live watcher self-test, every milestone gated by an independent build
> verifier and a spec audit); several real-device checks remain a manual
> pass (see [Limitations](#limitations) and the checklist in
> `TESTING.md`).

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
opaque plate — and whenever it can't verify it's protecting you, it
says so with a small on-output status chip instead of guessing.

## How it works, and why

**Deterministic, never AI.** Detection is Win32 window enumeration plus
UI Automation — window class/process signatures and the UIA password
property. There is no machine learning anywhere in this plugin. A privacy
guarantee you can't audit isn't a guarantee; every masking decision here
is traceable to an explicit OS-level rule.

**Masks only when confident — and says so when it isn't.** A single
watcher thread owns detection and updates a heartbeat every tick. Masks
are drawn only when both the detection and the coordinate mapping are
confident; StreamSentry never guesses a mask position and never
disrupts your output. If the heartbeat goes stale (>500 ms), the
watcher dies, or the capture geometry can't be resolved, your stream
keeps rendering normally and a small opaque **"protection degraded"
chip** appears on the output (plus an OBS log line) until protection
verifies again. The chip **cannot be turned off** — you always know
when you are unprotected. (Design decision 2026-07-09: a wrong or
disruptive mask is worse than a missed one; this replaced the v0.1
full-blackout behavior.)

**Opaque, never blur.** Masks are solid, opaque plates — a notification
card for toasts, a dark privacy plate (lock icon) for windows and password
fields. StreamSentry **never blurs or pixelates**, because blur and mosaic
are *reversible*: an archived clip can be attacked offline with deblurring
tools. Opaque is the only honest guarantee.

## Requirements

- Windows 10 21H2+ or Windows 11, x64
- OBS Studio 30 or newer

## Install

1. Download `streamsentry-0.2.0-windows-x64.zip` from the Releases page.
   It contains one folder, `streamsentry\`, laid out as a self-contained
   OBS plugin (`bin\64bit\streamsentry.dll` + `data\locale\...`).
2. Install it either way:
   - **Plugin folder (recommended, OBS 28+):** extract the `streamsentry`
     folder into `%ProgramData%\obs-studio\plugins\` (create `plugins` if
     it does not exist).
   - **Into the OBS install directory:** copy
     `streamsentry\bin\64bit\*` into `obs-studio\obs-plugins\64bit\` and
     `streamsentry\data\*` into `obs-studio\data\obs-plugins\streamsentry\`.
3. Restart OBS. Confirm `obs-studio\...\logs` shows
   `[streamsentry] plugin loaded successfully`.

A self-built DLL may trigger a Windows SmartScreen warning; that is normal
for locally-built, unsigned binaries.

## Usage

1. Add a **Display Capture** source (the source type supported for
   coordinate mapping — see Limitations).
2. Right-click it → **Filters** → under **Effect Filters** add
   **StreamSentry**.
3. Settings:
   - **Enable** — turns the whole filter on/off.
   - **Masking mode** (v0.2):
     - **Blocklist** (default) — mask only the windows you list.
     - **Allowlist** — mask **everything except** the windows you list.
       Default-deny: an empty allowlist masks the whole screen
       (including the taskbar and wallpaper) until you approve things.
       This is the structural answer to "the thing I never thought to
       blocklist" — nothing shows unless you said so.
   - **Blocklist / Allowlist** — one process name or window-title
     substring per line (case-insensitive). The blocklist comes
     pre-filled with sensible defaults (password managers and
     credential dialogs; clearing it restores them). The allowlist
     starts empty on purpose. The two lists are stored separately —
     switching modes never reinterprets one as the other.
   - **Window picker** (v0.2) — the *Open windows* dropdown lists what
     is currently on screen as "process — title". Pick one, press *Add
     selected window to the active list*, and it lands in whichever
     list your current mode uses. No more guessing process names.
     *Refresh window list* re-scans. Duplicates are not added twice.
   - There is deliberately **no** option to disable the "protection
     degraded" status chip.
4. **Panic hotkey** (v0.2): bind "StreamSentry: mask everything
   (panic)" under OBS **Settings → Hotkeys**. Press once → the entire
   source is replaced by an opaque privacy plate on the next frame;
   press again → back to normal. Not persisted: a fresh OBS session
   always starts un-panicked.

Toast masking and password-field masking are always on while the filter
is enabled, in both modes — approving a process in the allowlist does
NOT exempt its notification toasts.

## Limitations

StreamSentry is honest about what it does not do. As of v0.2:

- **Masking works on unscaled full-monitor Display Capture.** A
  **Window Capture**, a scaled/cropped capture, or two monitors of
  *identical resolution* cannot be mapped with certainty — those
  sources render normally, and while something needs masking the
  "protection degraded" chip shows instead (StreamSentry never guesses
  a mask position).
- **Toast matching is geometry-narrowed.** On Windows 11 the toast host
  window class is shared with other XAML popups (Start-menu search,
  taskbar flyouts); a geometry gate (right-edge band + size bounds)
  keeps notification cards off those. Some right-edge flyouts of
  toast-like size may still get a card.
- **UAC / credential prompts on the secure desktop** are not capturable by
  OBS at all, so they cannot appear in your stream to begin with (the
  credential broker on the normal desktop *is* covered by the blocklist).
- **Allowlist masks by window rectangle, not by visible stacking
  order.** A maximized *unapproved* window sitting **behind** an
  approved one still masks its whole rectangle — so if any unapproved
  window is maximized, the output can be fully plated even though an
  approved window is on top. This is the safe direction (the hidden
  window would be exposed the moment you switch to it). Allowlist mode
  works best when you keep only approved apps open and minimize/close
  the rest.
- **Apps that draw their own popup notifications** (LINE, BitComet,
  many Electron apps) do not use Windows toasts, so toast masking
  cannot see them — cover the app with the blocklist (or leave it off
  your allowlist) instead.
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
- **UI is English-only** (localization is out of scope).

Performance measured on the reference machine (v0.1 baseline): watcher
thread ~0.55% of one core, added render cost ~0.03 µs/frame; v0.2 adds
a PID-name cache to remove the per-window process queries that caused
rare heartbeat stalls under streaming load (re-measurement is on the
acceptance checklist).

## Roadmap (not in v0.2)

Deliberately deferred: window-capture geometry support (the top
candidate for v0.3 — masks on Window Capture sources instead of the
degraded chip), Focus Assist / Do-Not-Disturb auto-integration, macOS
support, per-app policies. Never planned: blur/mosaic options, AI
detection, content secret scanning, a tray icon, auto-update.

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
[ARCHITECTURE.md](ARCHITECTURE.md) (how it is actually built, including
the multi-agent gate workflow — verifier / spec-guardian / scribe —
that every milestone must pass), [SPEC.md](SPEC.md) (what it must do,
and what is deliberately out of scope), and [TESTING.md](TESTING.md)
(what has been verified, and how).

**Everything else is internal project process** — you can safely ignore
it: [CLAUDE.md](CLAUDE.md) (iron rules for the AI-assisted workflow that
builds this plugin), [SETUP.md](SETUP.md) (dev-box bootstrap notes,
Traditional Chinese, machine-specific), [reports/](reports/) (frozen
per-milestone verification evidence — see its own README),
`.claude/` (workflow agent definitions), `scripts/` (dev environment
bootstrap).

## AI authorship disclosure

In the interest of transparency: **StreamSentry is a predominantly
AI-authored project.** An AI coding assistant (Anthropic's Claude, via
Claude Code) authored the architecture, the C/C++ source, the unit
tests, the CMake build configuration, and the documentation — working
through an AI-run multi-agent gate workflow, where an independent build
**verifier** and a spec/rules **auditor** reviewed every milestone.

The human maintainer *directed* the project rather than writing its
code: setting the requirements and constraints, making every design and
policy decision (for example the 2026-07-09 "assist, not insurance"
repositioning and the blocklist match semantics), and performing
acceptance and real-device verification.

This is stated plainly because it is the truth and because distribution
venues increasingly — and reasonably — ask for it. Concretely: platforms
whose rules prohibit "entirely or mostly AI-made" resources (the **OBS
Forum resource policy**, updated 2026-07-12, is one) would consider
StreamSentry ineligible on that basis. It is therefore distributed only
from its own repository / release page, **not** submitted to the OBS
Forum. Personal use and self-distribution are unaffected.

"Original code" below means original to this project — written against
the OBS Studio and Windows SDK documentation, not copied or adapted from
any other codebase — not that it was human-written.

## License

GPL-2.0-or-later — see [LICENSE](LICENSE). All original code, written
against the OBS Studio and Windows SDK documentation.
