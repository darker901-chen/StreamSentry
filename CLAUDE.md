# CLAUDE.md — Project Constraints

## What this is
A Windows OBS Studio video filter plugin — "accident insurance for screen capture." It deterministically masks the three classic on-stream accidents: (1) notification toasts popping up with private message content, (2) accidentally exposed sensitive windows (blocklist; v0.2 adds an inverted **allowlist mode** — mask everything except approved windows — plus a panic hotkey), (3) password managers / system credential dialogs, plus password-field focus as a secondary guard. Detection is OS-level (window enumeration + UI Automation). Fail-closed: if detection health cannot be verified, the entire filter output goes black.

## Iron rules (never violate)
1. **Fail-closed is the product.** Any uncertainty in detection health, coordinate mapping, or thread liveness → full-source blackout. Never fail-open. The fail-closed behavior must NOT be user-disableable.
2. **Deterministic only.** No ML/AI detection of any kind in this codebase.
3. **Masking is opaque — never blur, never pixelate/mosaic.** Reversible obfuscation is forbidden (archived clips can be attacked offline). Two styles: (a) notification-shaped placeholder card (rounded rect, bell icon, "Notification hidden") for toast rects; (b) privacy plate (dark fill + lock icon + "Hidden") for window/field rects. Raw black is reserved solely for the fail-closed state.
4. **No third-party dependencies** beyond what obs-plugintemplate provides. Windows SDK (UIA, COM) and libobs only.
5. **Scope is locked to v0.2** (see SPEC.md Part 2; v0.1 baseline + the five approved v0.2 items only). Do not implement backlog items (macOS, notifications, blur options, per-app policy, Focus Assist integration, tray icon, auto-update) even if they seem easy.
6. **License is GPLv2.** Specifically GPL-2.0-or-later, matching OBS Studio and obs-plugintemplate convention (decided 2026-07-05). All code written here is original, written against OBS APIs and Windows SDK documentation only. Never import or adapt code from any other private project.

## Architecture (fixed — as-built detail in ARCHITECTURE.md)
- One watcher thread (COM MTA) owns all detection: Win32 window enumeration on a 100–200ms timer for toasts + block/allowlist, plus UIA focus-changed events for password fields only. Toast signature = **process name AND window class** (both must match; check DWMWA_CLOAKED for UWP windows; v0.2 adds a geometry gate). Block/allowlist matching = case-insensitive substring vs **process image name OR window title** (owner ruling FINAL, commit a434b18). Event handlers do minimal work: copy rects, post to shared state, return.
- Shared state between watcher and render: rect list + heartbeat timestamp. Render-side critical section must be non-blocking and tiny.
- OBS video filter render callback: reads shared state; healthy → draw plates on rects; unhealthy (heartbeat > 500ms) or any mapping failure → full black + status text.
- Coordinate chain: UIA screen coords → capture source coords → canvas coords. Handle multi-monitor and per-monitor DPI. When uncertain, expand mask rect by 8–16px padding (over-mask, never under-mask).

## Workflow
- Before writing code for any step: state the plan, wait for confirmation.
- OBS 30+ APIs only; when unsure about an API, consult official OBS docs/headers in-repo rather than guessing.
- Every feature lands with a manual test note appended to TESTING.md.
