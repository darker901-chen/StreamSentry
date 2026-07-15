# CLAUDE.md — Project Constraints

## What this is
A Windows OBS Studio video filter plugin — a **privacy assist for screen capture** (owner repositioning 2026-07-09: assist, not insurance). It deterministically masks the three classic on-stream accidents: (1) notification toasts popping up with private message content, (2) accidentally exposed sensitive windows (blocklist; v0.2 adds an inverted **allowlist mode** — mask everything except approved windows), (3) password managers / system credential dialogs, plus password-field focus as a secondary guard. Detection is OS-level (window enumeration + UI Automation). When protection cannot be verified, the output keeps rendering and the user is clearly told the guard is inactive — the plugin never disrupts the stream.

## Iron rules (never violate)
1. **Never disrupt the user's output; mask only on confidence.** (Owner ruling 2026-07-09, reports/RULING-2026-07-09-fail-open.md — supersedes the v0.1 fail-closed-blackout rule.) Wrong masking is worse than under-masking. Masks are drawn only when detection AND coordinate mapping are confident. When protection cannot be verified (stale heartbeat, dead watcher, unresolved geometry, mapping failure): render the source unmodified, surface a clear failure notice (small opaque on-output status chip + OBS log) — never a full-frame blackout, never a guessed mask. Exception by mode design: allowlist mode fails toward its own default (mask-all), since default-deny is what that user opted into. Known non-content window exclusions (≤16px slivers and `Progman`/`WorkerW` wallpaper hosts) are fixed by owner ruling `reports/RULING-2026-07-13-noncontent-window-exclusions.md`; the taskbar and application windows are not exempt.
2. **Deterministic only.** No ML/AI detection of any kind in this codebase.
3. **Masks that ARE drawn are opaque — never blur, never pixelate/mosaic.** Reversible obfuscation is forbidden (archived clips can be attacked offline). Two styles: (a) notification-shaped placeholder card (rounded rect, bell icon, "Notification hidden") for toast rects; (b) privacy plate (dark fill + lock icon + "Hidden") for window/field rects. If plate texture creation fails for a confidently mapped rect, fall back to a solid opaque fill — never drop a confident mask.
4. **No third-party dependencies** beyond what obs-plugintemplate provides. Windows SDK (UIA, COM) and libobs only.
5. **Scope is locked to v0.2** (see SPEC.md Part 2; v0.1 baseline + the four shipping v0.2 items only). The experimental panic hotkey was removed by owner ruling on 2026-07-15. Do not implement backlog items (macOS, notifications, blur options, per-app policy, Focus Assist integration, tray icon, auto-update) even if they seem easy.
6. **License is GPLv2.** Specifically GPL-2.0-or-later, matching OBS Studio and obs-plugintemplate convention (decided 2026-07-05). All code written here is original, written against OBS APIs and Windows SDK documentation only. Never import or adapt code from any other private project.

## Architecture (fixed — as-built detail in ARCHITECTURE.md)
- One watcher thread (COM MTA) owns all detection: Win32 window enumeration on a 100–200ms timer for toasts + block/allowlist, plus UIA focus-changed events for password fields only. Toast signature = **process name AND window class** (both must match; check DWMWA_CLOAKED for UWP windows; v0.2 adds a geometry gate). Block/allowlist matching = case-insensitive substring vs **process image name OR window title** (owner ruling FINAL, commit a434b18). Event handlers do minimal work: copy rects, post to shared state, return.
- Shared state between watcher and render: rect list + heartbeat timestamp. Render-side critical section must be non-blocking and tiny.
- OBS video filter render callback: reads shared state; healthy → draw plates on confidently mapped rects; unverified (heartbeat > 500ms, unresolved geometry, mapping failure) → render source unmodified + failure status chip + log (owner ruling 2026-07-09; blackout removed).
- Coordinate chain: UIA screen coords → capture source coords → canvas coords. Handle multi-monitor and per-monitor DPI. Confident detections get 8–16px padding (placement tolerance around a known threat — not guess-masking).

## Workflow
- Before writing code for any step: state the plan, wait for confirmation.
- OBS 30+ APIs only; when unsure about an API, consult official OBS docs/headers in-repo rather than guessing.
- Every feature lands with a manual test note appended to TESTING.md.
- For implementation milestones and release preparation, use a sequential hub-and-spoke gate: the hub alone edits source; a verifier builds/tests and writes `reports/verify-NNN.md`; a spec-guardian audits the same diff and writes `reports/spec-review-NNN.md`; only after VERIFIED + PASS may a scribe update TESTING.md and CHANGELOG.md. A post-gate change invalidates the prior gate.
- Never push, publish, or submit externally without explicit owner authorization.
