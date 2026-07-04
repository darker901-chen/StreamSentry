# SPEC.md — v0.1 Technical Specification

## Platform
- Windows 10 21H2+ / Windows 11, x64. OBS Studio 30+.
- Form: **video filter** (attached to Display Capture / Window Capture sources).

## Detection (UIA watcher thread) — priority order
1. **Notification toasts (primary scenario)**: Windows toast notifications are OS windows with known process/class signatures (e.g. ShellExperienceHost-hosted CoreWindow). Detect via the same window-enumeration timer; report full toast rect. Ship these rules built-in and always-on while the filter is enabled. Verify exact class/process signatures on Win10 21H2+ and Win11 during Week 2 and record them in code comments.
2. **Sensitive-window blocklist (accidental window exposure)**: default list = password managers and system credential dialogs only (1Password, KeePass, Bitwarden, Windows credential/UAC dialogs). User-editable list of process names / window-title substrings. Enumerate on a 100–200ms timer; report full window rects.
3. **Password fields (secondary scenario)**: subscribe to UIA focus-changed events; if focused element has `UIA_IsPasswordPropertyId == TRUE`, report its `BoundingRectangle`. Value: guards against "show password" toggles, autofill dropdown reveals, and legacy plaintext apps. Not the headline feature.
4. **Heartbeat**: watcher updates a timestamp every tick. Render side treats heartbeat older than 500ms as unhealthy.

## Masking (render callback)
- Healthy + rects present, two opaque styles by rect type:
  - **Toast rects**: draw a **notification-shaped placeholder card** — rounded rect sized to the toast, bell icon, "Notification hidden" label. Reads as a native feature, not a glitch.
  - **Window / password-field rects**: draw a **privacy plate** — dark solid fill + lock icon + "Hidden" label.
- **Never blur, never pixelate/mosaic** — archived clips can be attacked offline with deblur/depixelation tools; opaque is the only honest guarantee. Raw black is reserved solely for the fail-closed state.
- Unhealthy / mapping failure / watcher thread death: **entire filter output black** + overlay text "Privacy guard: detection unavailable — output blocked".
- Rule: any uncertainty = black. Over-mask with 8–16px padding around rects.
- Toast windows are UWP-hosted: a toast window can exist while DWM-cloaked (not actually visible). Check `DWMWA_CLOAKED` + visibility before reporting, but when in doubt, report (over-mask).

## Coordinate mapping
Screen coords → source coords → canvas coords. Must handle:
- Multiple monitors with mixed resolutions
- Per-monitor DPI scaling (DPI awareness v2)
- Display Capture vs Window Capture origin differences
- Source transforms (scale/crop) applied in OBS

## Settings UI (minimal)
- Enable/disable checkbox
- Blocklist: multi-line text box (one process name or title substring per line)
- No option to disable fail-closed behavior

## Out of scope for v0.1
macOS; blur/mosaic; per-app policies; **allowlist mode** (only-approved-apps-visible — v0.2 headline); Focus Assist auto-DND integration (v0.2); content-level secret detection (.env / API keys — impossible deterministically, permanently out; documented as a limitation); AI detection of any kind; tray icon; auto-update; localization; OBS < 30.

## Known limitations (document, don't fix in v0.1)
- Apps with custom-drawn UI (some Electron apps, games) may not expose UIA password properties → covered by blocklist, documented in README.
- Chromium browsers enable their accessibility tree on demand; verify per-browser and record in a support matrix.
- Fullscreen-exclusive games unsupported.
- Millisecond-level gap between focus event and render application (1–2 frames theoretical exposure) — document honestly.
- IME candidate windows (CJK input) not handled.
- RDP / VM environments unsupported.
- Performance target: watcher thread < 1% of one core; measure and document.

## Acceptance test matrix (must pass before release)
| Scenario | Expected |
|---|---|
| Windows toast notification appears (test via LINE/Slack/Outlook desktop message) | Toast region plated before content is readable |
| Password field focused in Chrome/Edge/Firefox login page | Field region masked |
| Windows credential dialog / UAC | Masked or full-window plate |
| 1Password / KeePass window open | Full-window plate (blocklist) |
| Password field on second monitor with different DPI | Correct coordinates |
| Source has scale/crop transform | Mask lands correctly |
| Kill UIA watcher thread (fault injection) | Full black within ≤500ms + status text |
| Toggle plugin during 60fps recording | No visible frame drops (measure render time) |
| 2-hour idle run | No memory leak (stable working set) |
| Empty blocklist + no password field | Fully transparent pass-through |
