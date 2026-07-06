# SPEC.md — Technical Specification (v0.1 shipped + v0.2 additions)

Part 1 (v0.1) is the shipped baseline and stays as the historical
record; where the as-built implementation superseded a detail, the
note is inline. Part 2 specifies the v0.2 additions approved by the
owner 2026-07-05 (reports/V02-PLAN.md). Current locked scope =
v0.1 baseline + the v0.2 items below, nothing else.

# Part 1 — v0.1 (shipped)

## Platform
- Windows 10 21H2+ / Windows 11, x64. OBS Studio 30+.
- Form: **video filter** (attached to Display Capture / Window Capture sources).

## Detection (UIA watcher thread) — priority order
1. **Notification toasts (primary scenario)**: Windows toast notifications are OS windows with known process/class signatures (e.g. ShellExperienceHost-hosted CoreWindow). Detect via the same window-enumeration timer; report full toast rect. Ship these rules built-in and always-on while the filter is enabled. Verify exact class/process signatures on Win10 21H2+ and Win11 during Week 2 and record them in code comments. *(As built: on Win11 26200 the verified signature is `explorer.exe` + `Xaml_WindowedPopupClass` — the ShellExperienceHost example above is superseded; see watcher.cpp comments. Known to over-match other explorer flyouts; narrowed in v0.2 §2.2.)*
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

# Part 2 — v0.2 additions (approved 2026-07-05)

Driven by owner field-testing of v0.1 (findings in
reports/V02-PLAN.md). Five items, mapped to milestones M6–M8. All v0.1
iron rules apply unchanged; in particular every feature below must
preserve fail-closed semantics and opaque-only masking.

## 2.1 Watcher performance hardening (M6)

Problem: rare fail-closed blips (`heartbeat stale (age 501-505 ms)`,
clearing ~100–150ms later, ~1/minute) while SRT-streaming. Suspected
cost: per-tick `OpenProcess` + `QueryFullProcessImageNameW` for every
visible window.

- **PID→image-name cache** in the watcher. Key: PID. An entry is
  trusted only while its PID is observed in consecutive ticks; any PID
  absent for a full tick is evicted, so a reused PID re-queries. (PID
  reuse across a one-tick gap is the residual race — document it; the
  gap between process exit and a new window appearing is ≥ one 150ms
  tick in practice.)
- **Tick-duration instrumentation** under `STREAMSENTRY_PERF_LOG`:
  record avg + max + p99 tick duration; additionally (always compiled,
  not just perf builds) log one warning when a single tick exceeds
  250ms — an early signal at half the stale threshold.
- **Watcher thread priority**: measure first; if p99 under streaming
  load still approaches the threshold after the cache,
  `THREAD_PRIORITY_ABOVE_NORMAL` is the approved second lever.
- Heartbeat semantics unchanged: written only with a completed
  publish. Do NOT touch the heartbeat mid-tick to paper over slow
  ticks (`ss_state_touch_heartbeat` stays caller-free).
- Acceptance: 30-min streaming soak with a busy desktop → zero
  stale-heartbeat fail-closed events; measured tick p99 recorded in
  reports/ (target: p99 ≤ 50ms).

## 2.2 Toast-match narrowing by geometry (M6)

Problem: on Win11, `explorer.exe + Xaml_WindowedPopupClass` also
matches Start-search/taskbar flyouts → up to 9–27 plates flapping.

- Keep the process+class signature as the base gate. Add a
  deterministic **geometry gate**: candidate must (a) lie within the
  toast spawn band of *some* monitor — the vertical edge band where
  Windows spawns toast banners, verified empirically per Windows build
  family and recorded in code comments (same verification discipline
  as the v0.1 signature); (b) have plausible toast dimensions, with
  **generous** bounds (size envelope wide enough for accent/inline
  variants).
- Over-mask-safety argument (required in code comments): narrowing
  trades false-positive plates for false-negative risk. Bounds must be
  generous, spawn band spans the full right edge (covers top-right and
  bottom-right placements), and the acceptance test must show a real
  toast is still masked before content is readable.
- Acceptance: open Start search, taskbar flyouts, tray overflow → no
  plate storm; fire a real toast → masked before readable (the v0.1
  acceptance row still passes).

## 2.3 Allowlist mode (M7 — the headline)

Rationale: a blocklist cannot protect against the unanticipated (owner
hit a payment site with a plain-text sensitive field: no OS password
marking, not in any list). Structural fix: invert the default.

- **Mode switch** in filter settings: `Blocklist (mask listed
  windows)` — the v0.1 behavior, remains the default on upgrade — vs
  `Allowlist (mask everything except listed)`.
- Allowlist semantics: every visible, non-cloaked top-level window
  that does **not** match the allowlist is reported as a
  `SS_RECT_WINDOW` mask rect. Match semantics identical to the
  blocklist (case-insensitive substring vs process image name OR
  window title — owner ruling a434b18 applies to both lists).
- **Toast and password-field detection stay active in both modes.**
  Approving a process (e.g. explorer.exe for the taskbar) must NOT
  exempt its toasts: toast rects keep their own kind and card. A
  window matching the allowlist is exempt only from the
  default-mask-everything rule.
- No implicit approvals: shell surfaces (taskbar, desktop) are windows
  like any other and start masked. First-run consequence (taskbar
  plated until approved) is acceptable default-deny; the M8 picker
  makes approving them one click.
- **Rect-budget overflow**: if maskable windows exceed `SS_MAX_RECTS`,
  the watcher publishes a `mask_all` flag in the snapshot; the render
  side then draws one full-source **privacy plate** (opaque, lock +
  "Hidden" — NOT raw black, which stays reserved for fail-closed).
  Over-mask, never drop rects silently.
- Fail-closed semantics **unchanged** in allowlist mode: stale
  heartbeat / unresolved geometry / mapping failure → full black +
  banner, exactly as v0.1. (Allowlist only changes *which* rects are
  reported, not health semantics.)
- Storage: `blocklist` and `allowlist` are **separate settings keys**;
  switching modes must never reinterpret one list as the other
  (inverted meaning would be a security bug). Allowlist default:
  empty.

## 2.4 Panic hotkey (M7)

- One OBS hotkey per filter instance (registered via
  `obs_hotkey_register_source`, configured in OBS Settings → Hotkeys;
  no properties-UI element): **"StreamSentry: mask everything
  (panic)"**, toggle semantics.
- Engaged → render a full-source **privacy plate** (opaque, lock +
  "Hidden") instead of the target; released → normal pipeline.
  Distinct from fail-closed black so the streamer can tell "I pressed
  panic" from "detection died" at a glance; if both apply, fail-closed
  (black + banner) wins so health stays observable.
- Not persisted across OBS sessions: a fresh session starts with panic
  released (a forgotten invisible global mask across sessions
  surprises the user; fail-closed — not panic — is the safety net).
- Acceptance: hotkey engages within one frame (mask visible on the
  very next rendered frame), toggles cleanly, works in both modes, and
  fail-closed still overrides it.

## 2.5 Window-picker UI (M8)

Problem: the blocklist textbox is developer-grade — users cannot know
names like `credentialuibroker`.

- A **"List open windows"** button in filter properties. On click,
  enumerate current visible top-level windows (same
  visibility/cloaking gates as the watcher) and present process image
  name + window title per entry; the user selects entries and they are
  appended to the **active** list (blocklist or allowlist per mode) as
  process-name lines — no hand-typing. Exact widget shape is
  implementation-defined within stock OBS properties (no custom Qt
  dependency — iron rule 4).
- The multiline textbox stays (power users, removals, title-substring
  entries); the picker only appends.
- Acceptance: with a target app open, a user adds it to the active
  list via the picker alone and the mask appears/passes accordingly;
  duplicate selections do not duplicate entries.

## 2.6 Chromium password-field investigation (stretch, non-gating)

v0.1 documented limitation: Chromium browsers keep the accessibility
tree off until an assistive client is detected, so UIA focus events
for password fields may not fire. v0.2 work item: investigate
per-browser (Chrome/Edge/Firefox current stable), record a support
matrix in README/TESTING. **Documentation-only unless a deterministic,
dependency-free activation exists**; no speculative code. This item
does not gate the v0.2 release.

## Out of scope for v0.2

Everything in the v0.1 out-of-scope list that is not explicitly pulled
in above stays out — notably: macOS; blur/mosaic (permanently, iron
rule 3); per-app policies; Focus Assist auto-DND integration
(mentioned as a v0.2 candidate in the v0.1 spec, but NOT in the
approved plan — it stays backlog); content-level secret detection
(permanently); AI detection (permanently); tray icon; auto-update;
localization; OBS < 30; window/game-capture geometry support.

## v0.2 acceptance additions (on top of the v0.1 matrix)

| Scenario | Expected |
|---|---|
| 30-min SRT streaming soak, busy desktop | Zero stale-heartbeat fail-closed events; tick p99 ≤ 50ms recorded |
| Start search / taskbar flyouts / tray overflow opened | No toast-plate storm |
| Real toast during the above | Still masked before content readable |
| Allowlist mode, one approved app | Approved app visible; every other window plated |
| Allowlist mode, > `SS_MAX_RECTS` unapproved windows | Single full-source privacy plate (not black) |
| Allowlist mode, watcher killed (fault injection) | Full black + banner ≤ 500ms (unchanged) |
| Mode switch round-trip | Blocklist and allowlist contents both survive untouched |
| Panic hotkey pressed / released | Full-source plate next frame / normal render resumes |
| Panic + watcher killed | Fail-closed black + banner wins |
| Picker: add open window, no typing | Entry appended to active list; mask behavior updates |
