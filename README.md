# StreamSentry

**A privacy assist for screen capture.** A Windows OBS Studio video
filter that deterministically masks the classic on-stream privacy
accidents *before* they reach your capture — and clearly tells you
(with an on-output status chip) whenever it cannot verify protection.
Blocklist mode keeps the source rendering on those failures; allowlist mode
uses the full-source opaque fallback that its default-deny design promises.

> Status: **v0.2.0 beta.** Machine-verified (4 unit-test suites +
> live watcher self-test, every milestone gated by a separate build-verifier
> agent and spec-auditor agent); several real-device checks remain a manual
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

**Deterministic runtime detection — no AI.** Detection is Win32 window enumeration plus
UI Automation — window class/process signatures and the UIA password
property. The installed plugin performs no machine-learning inference and
sends no captured content to a service; every masking decision is traceable
to an explicit OS-level rule. Development authorship is disclosed below.

**Masks only when confident — and says so when it isn't.** A single
watcher thread owns detection and updates a heartbeat every tick. Masks
are drawn only when both the detection and the coordinate mapping are
confident; StreamSentry never guesses a mask position. If the heartbeat
goes stale (>500 ms), the watcher dies, or the capture geometry cannot be
resolved, a small opaque **"protection degraded" chip** appears on the
output (plus an OBS log line) until protection verifies again. In
**Blocklist** mode the source keeps rendering normally; in **Allowlist** mode
the entire source becomes an opaque privacy plate because that mode is
explicitly default-deny. The chip **cannot be turned off**. (Design decision
2026-07-09: a wrong or disruptive guessed mask is worse than a missed one;
allowlist's mask-all fallback is the deliberate mode exception.)

**Opaque, never blur.** Masks are solid, opaque plates — a notification
card for toasts, a dark privacy plate (lock icon) for windows and password
fields. StreamSentry **never blurs or pixelates**, because blur and mosaic
are *reversible*: an archived clip can be attacked offline with deblurring
tools. Opaque is the only honest guarantee.

## Requirements

- Windows 10 21H2 or later, or Windows 11, x64
- OBS Studio 32.1.2 (verified). Older OBS releases may work but are not part
  of the current compatibility evidence.

## Install — standard OBS setup (about 60 seconds)

1. On the [Releases page](https://github.com/darker901-chen/StreamSentry/releases),
   open the latest **0.2.0 beta** release and download
   `streamsentry-0.2.0-windows-x64.zip`. Do not download GitHub's automatic
   **Source code** archives; they do not contain the ready-to-use plugin DLL.
2. Close OBS completely.
3. Press **Win+R**, paste `%ProgramData%\obs-studio\plugins`, and press Enter.
   Create the `plugins` folder if Windows says it does not exist.
4. Open the downloaded zip and copy its `streamsentry` folder into that
   `plugins` folder. The final DLL path must be exactly:

   ```text
   C:\ProgramData\obs-studio\plugins\streamsentry\bin\64bit\streamsentry.dll
   ```

   If you see `streamsentry\streamsentry\bin`, move the inner folder up one
   level.
5. Start OBS. Add or select an unscaled full-monitor **Display Capture**,
   right-click it → **Filters** → under **Effect Filters** press **+** →
   **StreamSentry**.

For a custom-location or portable OBS installation, or if the filter does not
appear, use the exact-path checks in the
[full installation SOP](docs/INSTALLATION.md). It also includes checksum
verification, a two-minute masking smoke test, update, uninstall, and log-based
troubleshooting.

For log confirmation, use **Help → Log Files → View Current Log** or open
`%APPDATA%\obs-studio\logs`; look for
`[streamsentry] plugin loaded successfully (version 0.2.0)`.

The beta DLL is unsigned. Windows or security software may warn about the
download; verify the published checksum and do not bypass a warning if the
hash does not match.

### Uninstall

Close OBS, delete `%ProgramData%\obs-studio\plugins\streamsentry\`, then
start OBS again. Existing scenes may retain an unavailable-filter entry until
you remove that filter from the source.

### Quick install troubleshooting

- If StreamSentry is missing, confirm the exact DLL path from step 4 and check
  the current OBS log for `streamsentry` or a module-load error.
- The beta is Windows x64 only; Windows on Arm and 32-bit builds are unsupported.
- `Failed to load 'zh-TW' text` followed by the successful-load line means OBS
  fell back to the bundled English locale; it is not a plugin-load failure.
- Custom-location and portable OBS installs may need their own plugin paths;
  follow the full installation SOP instead of guessing or nesting folders.
- Installing directly into a standard OBS application directory is a legacy
  layout that OBS says will stop working in a future version, so it is not the
  recommended default.

## Usage

1. Add a **Display Capture** source (the source type supported for
   coordinate mapping — see Limitations).
2. Right-click it → **Filters** → under **Effect Filters** add
   **StreamSentry**.
3. Settings:
   - **Enable** — turns the whole filter on/off.
   - **Masking mode** (v0.2):
     - **Blocklist** (default) — mask only the windows you list.
     - **Allowlist** — mask every detectable content-bearing window except
       the windows you list. Default-deny: an empty allowlist masks application
       windows and the taskbar until you approve them. Known non-content
       desktop wallpaper hosts remain visible.
       This is the structural answer to "the thing I never thought to
       blocklist" — nothing shows unless you said so.
   - **Blocklist / Allowlist** — one process name or window-title
     substring per line (case-insensitive). The blocklist comes
     pre-filled with sensible defaults (password managers and
     credential dialogs; clearing it restores them). The allowlist
     starts empty on purpose. The two lists are stored separately —
     switching modes never reinterprets one as the other.
   - **Window picker** (v0.2) — the *Open windows* dropdown lists what
     is currently on screen as "process - title". Pick one, press
     **Add to list**, and it lands in whichever list your current mode
     uses. No more guessing process names. **Refresh list** re-scans.
     Duplicates are not added twice.
   - There is deliberately **no** option to disable the "protection
     degraded" status chip.
Toast masking and password-field masking are always on while the filter
is enabled, in both modes — approving a process in the allowlist does
NOT exempt its notification toasts.

## Limitations

StreamSentry is honest about what it does not do. As of v0.2:

- **Masking works on unscaled full-monitor Display Capture.** A
  **Window Capture**, a scaled/cropped capture, or two monitors of
  *identical resolution* cannot be mapped with certainty. In Blocklist mode
  those sources render normally with the "protection degraded" chip; in
  Allowlist mode they fall to a full-source opaque privacy plate plus the chip
  because allowlist is default-deny. StreamSentry never guesses a mask
  position.
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

**Using the plugin?** Start with the
[installation and first-run SOP](docs/INSTALLATION.md), then use this README as
the feature and limitations reference. Release changes are in
[CHANGELOG.md](CHANGELOG.md).

**Publishing a beta?** Follow the maintainer-only
[GitHub publishing SOP](docs/PUBLISHING_SOP.md). It keeps verification, the
downloadable artifact, repository visibility, and the public Release in the
required order.

**Reading or modifying the code?** Start with
[ARCHITECTURE.md](ARCHITECTURE.md) (how it is actually built, including
the multi-agent gate workflow — verifier / spec-guardian / scribe —
that every milestone must pass), [SPEC.md](SPEC.md) (what it must do,
and what is deliberately out of scope), and [TESTING.md](TESTING.md)
(what has been verified, and how).

**Everything else is internal project process** — you can safely ignore
it: [CLAUDE.md](CLAUDE.md) and [AGENTS.md](AGENTS.md) (iron rules for the
   Claude Code and Codex workflows that
build this plugin), [SETUP.md](SETUP.md) (dev-box bootstrap notes,
Traditional Chinese, machine-specific), [reports/](reports/) (frozen
per-milestone verification evidence — see its own README),
`.claude/` (workflow agent definitions), `scripts/` (dev environment
bootstrap).

## AI authorship disclosure

**StreamSentry is a predominantly AI-authored project.** Anthropic's Claude,
via Claude Code, authored the architecture, C/C++ source, tests, build files,
and documentation through a multi-agent workflow with separate verifier and
spec-auditor roles.

The human maintainer *directed* the project rather than writing its
code: setting the requirements and constraints, making every design and
policy decision (for example the 2026-07-09 "assist, not insurance"
repositioning and the blocklist match semantics), and performing
acceptance and real-device verification.

This project is distributed from its own repository and is not submitted to
the OBS Forum resource directory, whose current policy excludes resources
written entirely or largely with AI coding tools.

"Original code" below means original to this project — written against
the OBS Studio and Windows SDK documentation, not copied or adapted from
any other codebase — not that it was human-written.

## Support and issue reports

Use [GitHub Issues](https://github.com/darker901-chen/StreamSentry/issues) for
bugs and compatibility reports. Include the OBS version, Windows version,
capture-source type, reproduction steps, and the relevant StreamSentry log
lines. Do not attach logs containing private stream keys or account data.

## License

GPL-2.0-or-later — see [LICENSE](LICENSE). All original code, written
against the OBS Studio and Windows SDK documentation.
