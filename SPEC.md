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
- **Never blur, never pixelate/mosaic** — archived clips can be attacked offline with deblur/depixelation tools; opaque is the only honest guarantee. Raw black is reserved solely for the fail-closed state. *(Raw-black reservation obsolete since §2.7 removed the blackout.)*
- Unhealthy / mapping failure / watcher thread death: **entire filter output black** + overlay text "Privacy guard: detection unavailable — output blocked". *(SUPERSEDED 2026-07-09 by owner ruling — see Part 2 §2.7: render unmodified + failure chip; blackout removed.)*
- Rule: any uncertainty = black. Over-mask with 8–16px padding around rects. *(SUPERSEDED — §2.7: uncertainty = don't mask + tell the user; padding stays for confident detections only.)*
- Toast windows are UWP-hosted: a toast window can exist while DWM-cloaked (not actually visible). Check `DWMWA_CLOAKED` + visibility before reporting, but when in doubt, report (over-mask). *(SUPERSEDED by §2.7: when the cloak state cannot be queried the window is treated as cloaked and skipped — a plate over a window that is not actually displayed would be a wrong mask.)*

## Coordinate mapping
Screen coords → source coords → canvas coords. Must handle:
- Multiple monitors with mixed resolutions
- Per-monitor DPI scaling (DPI awareness v2)
- Display Capture vs Window Capture origin differences
- Source transforms (scale/crop) applied in OBS

## Settings UI (minimal)
- Enable/disable checkbox
- Blocklist: multi-line text box (one process name or title substring per line)
- No option to disable fail-closed behavior *(SUPERSEDED by §2.7: the blackout itself is removed; the failure status chip is likewise not disableable)*

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
| Kill UIA watcher thread (fault injection) | Full black within ≤500ms + status text *(§2.7: now source keeps rendering + status chip ≤500ms)* |
| Toggle plugin during 60fps recording | No visible frame drops (measure render time) |
| 2-hour idle run | No memory leak (stable working set) |
| Empty blocklist + no password field | Fully transparent pass-through |

# Part 2 — v0.2 additions (approved 2026-07-05)

Driven by owner field-testing of v0.1 (findings in
reports/V02-PLAN.md). Five items, mapped to milestones M6–M8, plus the
§2.7 repositioning ruled mid-v0.2. Iron rules apply as amended
2026-07-09: never disrupt the output, mask only on confidence (§2.7),
and drawn masks stay opaque-only.

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
  stale-heartbeat events (formerly fail-closed blackouts, §2.7: now
  protection-degraded chip occurrences); measured tick p99 recorded in
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
- Allowlist semantics: every visible, non-cloaked, content-bearing top-level
  window that does **not** match the allowlist is reported as a
  `SS_RECT_WINDOW` mask rect. Match semantics identical to the
  blocklist (case-insensitive substring vs process image name OR
  window title — owner ruling a434b18 applies to both lists).
- **Toast and password-field detection stay active in both modes.**
  Approving a process (e.g. explorer.exe for the taskbar) must NOT
  exempt its toasts: toast rects keep their own kind and card. A
  window matching the allowlist is exempt only from the
  default-mask-everything rule.
- No implicit process/title approvals: the taskbar and application windows
  start masked. Per owner ruling 2026-07-13, deterministic known-non-content
  windows are excluded before matching: windows ≤16px wide or tall, plus
  `Progman`/`WorkerW` desktop wallpaper hosts. An empty allowlist therefore
  approves no content-bearing window, while the wallpaper remains visible.
  The M8 picker makes approving the taskbar or an application one click.
- **Rect-budget overflow**: if maskable windows exceed `SS_MAX_RECTS`,
  the watcher publishes a `mask_all` flag in the snapshot; the render
  side then draws one full-source **privacy plate** (opaque, lock +
  "Hidden"). Never drop rects silently — mask-all is this mode's
  default state, not a blackout.
- Failure semantics in allowlist mode (§2.7): stale heartbeat /
  unresolved geometry / mapping failure → full-source **mask-all
  plate** + status chip. Default-deny is what this user opted into,
  so the mode fails toward its own default — never raw black, never
  silent pass-through.
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
  "Hidden") instead of the target; released → normal pipeline. A
  deliberate user action outranks health state (§2.7): if protection
  is simultaneously unverified, the panic plate stays and the status
  chip is drawn on top of it, so the streamer still learns the guard
  is inactive.
- Not persisted across OBS sessions: a fresh session starts with panic
  released (a forgotten invisible global mask across sessions
  surprises the user).
- Acceptance: hotkey engages within one frame (mask visible on the
  very next rendered frame), toggles cleanly, works in both modes, and
  stays engaged during injected watcher death (chip on top).

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

## 2.7 Fail-open repositioning (owner ruling 2026-07-09, FINAL)

Ruling record: reports/RULING-2026-07-09-fail-open.md. The product is a
privacy **assist**: wrong masking is worse than under-masking, and the
plugin must never disrupt the user's output. Supersedes the v0.1
fail-closed blackout everywhere.

- **Mask only on confidence.** A mask (or an allowlist pass-through
  hole) is drawn only when detection and coordinate mapping are both
  confident. Nothing is ever drawn at a guessed position.
- **Unverified protection → render unmodified + tell the user.**
  Stale heartbeat (> 500ms), dead watcher, unresolved capture geometry
  (window capture, scaled capture, ambiguous monitors), or mapping
  failure: the source renders untouched; a small opaque status chip
  ("StreamSentry: protection degraded — see log") is drawn in a corner
  of the output and the condition is logged. Chip and log clear when
  protection verifies again. Full-frame blackout no longer exists.
  Watcher-side degradations that do NOT stall the heartbeat (e.g.
  monitor enumeration failure disabling the toast gate) publish a
  `detection_degraded` flag in the snapshot so the render side still
  shows the chip — **no degradation is ever silent**.
- **Confident masks keep all v0.1 guarantees**: opaque, padded,
  correct styles. If plate texture creation fails for a confident
  rect, fall back to a solid opaque fill (never drop a confident
  mask, never blur).
- **Allowlist mode exception (by design, not by uncertainty):**
  allowlist mode's failure state is its own default — mask-all —
  because default-deny is the behavior that user opted into. Approved
  holes are only punched with confident mapping.
- **Toast gate direction flips**: geometry-gate uncertainty (missing/
  incomplete monitor data, degenerate input) now classifies as NOT a
  toast — no mask — instead of over-masking.
- Acceptance-matrix changes: the v0.1 "kill watcher → full black
  ≤ 500ms" row becomes "kill watcher → source keeps rendering +
  status chip visible ≤ 500ms + log line". The v0.2 allowlist
  fault-injection row becomes "allowlist + watcher killed → mask-all
  plate (not black)".

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
| 30-min SRT streaming soak, busy desktop | Zero stale-heartbeat protection-degraded events; tick p99 ≤ 50ms recorded |
| Start search / taskbar flyouts / tray overflow opened | No toast-plate storm |
| Real toast during the above | Still masked before content readable |
| Allowlist mode, one approved app | Approved app visible; every other window plated |
| Allowlist mode, > `SS_MAX_RECTS` unapproved windows | Single full-source privacy plate (not black) |
| Allowlist mode, watcher killed (fault injection) | Full-source mask-all plate ≤ 500ms (§2.7: the mode's own default, not black) |
| Mode switch round-trip | Blocklist and allowlist contents both survive untouched |
| Panic hotkey pressed / released | Full-source plate next frame / normal render resumes |
| Panic + watcher killed | Panic plate stays (deliberate user action outranks health state); status chip on top |
| Picker: add open window, no typing | Entry appended to active list; mask behavior updates |
