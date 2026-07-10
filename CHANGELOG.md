# Changelog

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## Unreleased

Two unreleased sets live here, newest first: the v0.2 development cycle
(M5 onward) and, below it, the 0.1.0 release-candidate set (M0 through M4).
Nothing has been pushed or tagged; publishing is a human step.

### 0.2.0-m6.5 - 2026-07-10

Milestone M6.5 repositions the product's failure behavior on an owner product
ruling (2026-07-09, recorded FINAL with its Application addendum in
`reports/RULING-2026-07-09-fail-open.md`): the plugin is a privacy
**assist** — wrong masking is worse than under-masking and the plugin must
never disrupt the user's output — so the v0.1 fail-closed full-frame
blackout is removed. When protection cannot be verified, the source now
renders unmodified and the plugin tells the user instead: a small opaque
top-left status chip ("StreamSentry: protection degraded - see log") plus
one `PROTECTION DEGRADED: <reason>` warning in the OBS log per engagement,
clearing with a `protection restored` line. Masks are drawn only on
confidence, and confidently detected-and-mapped masks are never dropped —
not even on degraded frames. The change set at the final gate is 26 files;
no version bump (still builds as `streamsentry.dll` 0.1.0). Verified by
`reports/M6.5-verifier.md` (FINAL VERDICT: VERIFIED — the tree was amended
twice by gate findings and the full from-scratch sequence was re-run for
each revision, pinned by blob hash; the RUN-3 ADDENDUM is authoritative:
zero-warning builds in both `STREAMSENTRY_PERF_LOG` variants with the tree
left OFF, ctest 4/4 suites — 107 assertions, watcher-selftest deterministic
legs all pass with the expected INCONCLUSIVE toast leg) and
`reports/M6.5-spec-guardian.md` (ROUND-3 VERDICT: PASS; the report preserves
its first-pass FAIL — five violations, including the guardian's own
checklist still mandating blackout — and its round-2 FAIL — V6, the first
fix dropping confident masks — verbatim; `reports/GATE-CATCHES.md` indexes
these catches). After the PASS, three doc-only wording fixes from the
guardian's own non-blocking round-3 list were applied; no code changed after
the final verification run.

#### Added
- Pure per-frame decision module `src/frame-decide.c`/`.h`: the entire
  render-side decision (no-snapshot/stale → detection-degraded → geometry →
  per-rect mapping, first-trigger reason precedence) extracted from
  `src/filter.c` after guardian finding V6 so the logic is unit-testable; it
  includes no OBS or Windows headers.
- Fourth ctest suite `frame-decide-tests` (`tests/frame-decide-tests.c`, 25
  assertions) pinning the V6 regression — `detection_degraded` plus a
  confident rect must flag the frame (chip) WITHOUT dropping the mask — plus
  failure-reason precedence ordering, stale-heartbeat-drops-all-masks,
  INVALID-among-OK keeps the OK rect, and off-capture silent skip.
- Watcher `detection_degraded` flag in the published snapshot (guardian
  finding V1): monitor-enumeration failure or truncation — which leaves the
  toast gate unable to affirm — now surfaces the chip and logs the
  engage/clear transitions once each, instead of silently stopping toast
  masking while the heartbeat stays fresh. No degradation is silent.
- `reports/RULING-2026-07-09-fail-open.md`: the ruling record plus its
  Application addendum (the authority chain for the cloak-doubt flip, the
  chip-label generalization, and the guardian-checklist amendment — all
  flagged for owner countersign at the manual acceptance pass).

#### Changed
- Unverified protection (stale heartbeat > 500 ms, missing snapshot,
  unresolved capture geometry, mapping failure, filter-begin bypass) now
  renders the source unmodified with the status chip and the WARN/INFO log
  pair described above. Log-string migration for anyone grepping OBS logs:
  `FAIL-CLOSED engaged: ...` → `PROTECTION DEGRADED: ...`; the always-on
  slow-tick warning now reads "(early warning; detection-stale threshold is
  500 ms)"; the `PERF watcher tick` format is unchanged.
- Confident rects keep their opaque plates on degraded frames; plate-texture
  allocation failure for a confident rect now falls back to a solid opaque
  fill at the mapped rect instead of engaging a full-frame failure state.
- Toast geometry gate direction flipped: uncertainty (missing or incomplete
  monitor data, degenerate geometry) now classifies as NOT a toast — no
  mask — instead of M6's over-mask. The five uncertainty assertions in
  `tests/toast-gate-tests.c` are inverted, with the NaN fixture strengthened
  so the case stays discriminating under the new rule.
- Cloak-query failure direction flipped: a window whose `DWMWA_CLOAKED`
  state cannot be queried is treated as cloaked and skipped (was: reported /
  over-masked) — a plate over a window that is not actually displayed would
  be a wrong mask.
- Law and docs amended under the ruling's authority: CLAUDE.md iron rules 1
  and 3 rewritten; SPEC.md Part 2 gains §2.7 with superseded notes on the
  affected Part 1 lines and amended acceptance rows (the kill-watcher row is
  now "source keeps rendering + status chip ≤ 500 ms + log line");
  ARCHITECTURE.md failure semantics rewritten and the multi-agent gate
  workflow documented; README repositioned (assist, chip, no blackout
  claims); the spec-guardian agent checklist's "failure paths must land in
  blackout" bullet replaced by failure-notice integrity.

#### Removed
- The fail-closed full-frame blackout and its status banner: no path draws
  raw black anymore (`gs_clear` absent from `src/`; the old full-black fill
  is gone — guardian §2). The 500 ms staleness constant survives; it now
  gates the failure notice instead of a blackout.

#### Dependencies
- Unchanged: libobs + Windows SDK only (guardian §5); `CMakeLists.txt`
  changes are limited to wiring the new module and test target. The new
  test binary links only pure first-party sources.

#### Pending owner acceptance (binding for the v0.2 ship)
- In-OBS degraded-chip behavior: chip within ≤ 500 ms of watcher death,
  clears on recovery, and confident masks persist on degraded frames — the
  render-path pixels have no automated harness. See TESTING.md (M6.5) for
  the current checklist.
- The M6 soak and flyout rows carry forward with the renamed grep targets;
  the deferred real-toast items remain blocked while toast banners are
  system-suppressed on the dev machine.
- Owner countersign of the ruling's Application addendum items and the Q1
  per-window residual (a visible window whose cloak query persistently
  fails is skipped without a notice), as requested by the spec-guardian.

### 0.2.0-m6 - 2026-07-09

Milestone M6 is the first v0.2 code milestone: watcher performance hardening
(SPEC Part 2 item 2.1) and toast-match narrowing by geometry (SPEC Part 2
item 2.2) — the two fixes driven by the 2026-07-05 field findings
(heartbeat-stale fail-closed blips under streaming load; shell flyouts
sharing the toast window class and producing plate storms). The change set is
exactly seven files: `src/watcher.cpp`, new `src/toast-gate.c`/`.h`, new
`tests/toast-gate-tests.c`, `CMakeLists.txt`, `ARCHITECTURE.md`, and new
`reports/M6-toast-probe.txt`. No version bump; the module still builds as
`streamsentry.dll` 0.1.0. Verified by `reports/M6-verifier.md` (FINAL
VERDICT: VERIFIED, 7/7 checks — from-scratch configure + build, preset
`windows-x64-local` RelWithDebInfo, zero compiler/linker warnings in both
`STREAMSENTRY_PERF_LOG` variants with the tree left OFF; ctest 3/3 suites;
watcher-selftest 8/8 deterministic checks, toast leg INCONCLUSIVE because
banners are system-suppressed on the dev machine; its ADDENDUM is
authoritative — `src/watcher.cpp` was amended mid-verification and the full
sequence was re-run clean against the final tree) and
`reports/M6-spec-guardian.md` (Verdict: PASS on re-audit — one first-pass
residual fixed in-milestone, zero open code findings, both M5 audit
observations closed).

#### Added
- Toast geometry gate (`src/toast-gate.c`/`.h`, a pure module): after the
  v0.1 process-AND-class toast signature, a candidate must ALSO sit in the
  right-edge spawn band of some monitor (right edge within 160 px of the
  monitor's right edge; full monitor height, so top-right and bottom-right
  anchors and slide-in overhang all pass) and have plausible banner
  dimensions (width 200–1000 px and ≤ 60% of monitor width; height
  60–1200 px and ≤ 90% of monitor height). A window that fails the gate loses
  only the toast-card classification and still falls through to
  block/allowlist matching; any uncertainty (no or incomplete monitor data,
  degenerate geometry) classifies as toast — the over-mask direction, per
  iron rule 1. This narrows the v0.1 over-match in which Start-search and
  taskbar flyouts sharing the toast window class flapped 9–27 "Notification
  hidden" plates.
- The gate constants are PROVISIONAL: derived from documented Windows toast
  metrics (396-DIP banner width) with generous over-mask-safe bounds and
  derivations in code comments, because toast banners are currently
  system-suppressed on the dev machine and no live banner could be captured.
  An owner ruling (2026-07-09, recorded with the full diagnosis in
  `reports/M6-toast-probe.txt`) authorizes this for M6 and defers empirical
  calibration plus re-verification of the v0.1 toast signature on Windows
  build 26200.8655 to the owner acceptance pass — both remain binding for
  the v0.2 ship.
- New ctest suite `toast-gate-tests` (`tests/toast-gate-tests.c`, 23
  assertions): positive toast shapes at 100/150/200% DPI including slide-in
  animation and a negative-origin secondary monitor; negative fixtures
  recorded from live shell/app flyovers (probe record); multi-monitor
  any-semantics; uncertainty-must-mask cases (NULL/zero monitors, degenerate,
  NaN). The one admitted flyover shape (tray-overflow-sized, right-edge) is
  asserted as deliberate over-mask, so any future tightening is a conscious
  act.
- PID→image-name cache in the watcher (`src/watcher.cpp`): keyed by PID; an
  entry is evicted after one full tick of absence (a reused PID re-queries);
  failed lookups are never cached, so process-name matching keeps retrying;
  the residual PID-reuse race (reuse within one tick-to-tick window) is
  documented in code and does not affect title-substring matching.
- Always-compiled slow-tick warning: any watcher tick over 250 ms — half the
  500 ms fail-closed stale threshold — logs a warning in every build,
  independent of `STREAMSENTRY_PERF_LOG`. A compile-time constant, not a
  user setting.

#### Changed
- `STREAMSENTRY_PERF_LOG` instrumentation (still a CMake option, OFF by
  default and compiled out of the shipping build) now reports per-200-tick
  avg/max/p99 tick cost plus PID-cache hit/eviction counts, upgraded from
  M3's plain tick-cost logging.
- The watcher re-enumerates monitors every tick for the gate; if enumeration
  fails, truncates at the 16-monitor cap, or any `GetMonitorInfoW` call
  fails, the gate is handed an EMPTY monitor list, so every signature-matched
  window is masked as a toast (v0.1-parity over-mask). This guard was flagged
  by the spec-guardian's first pass (incomplete enumeration previously failed
  toward unmask in 17-plus-monitor or hot-unplug-race configurations) and
  fixed in-milestone; the re-audit confirms the over-mask direction.
- Watcher thread priority deliberately NOT raised (SPEC 2.1's measure-first
  sequencing): the approved second lever stays unused until the soak
  measurement demands it, recorded in a code comment at the thread-creation
  site. Heartbeat and fail-closed semantics are untouched by this milestone.
- `CMakeLists.txt`: `toast-gate.c` added to the plugin target and to
  `watcher-selftest`; new `toast-gate-tests` ctest executable registered.
- `ARCHITECTURE.md`: M6 as-built updates (gate module, cache,
  instrumentation, the provisional-constants record, and the owner soak
  checklist).

#### Pending owner acceptance (binding for the v0.2 ship)
- SPEC 2.1 acceptance row: a 30-minute streaming soak with zero
  stale-heartbeat fail-closed events and a measured tick p99 ≤ 50 ms recorded
  in `reports/`. A perf-instrumented build was deployed to the owner's OBS on
  2026-07-09 for this measurement; nothing in the M6 verification measured
  tick latency under load.
- SPEC 2.2 acceptance rows: no plate storm from Start search / taskbar
  flyouts, and a real toast still masked before its content is readable —
  plus the deferred calibration and signature re-verification above, blocked
  until toast banners display on the dev machine again. See TESTING.md (M6)
  for the exact checklist, including the banner-suppression diagnostic note.

### 0.2.0-m5 - 2026-07-06

Milestone M5 opens the v0.2 cycle: documentation and governance only — no
code, test, or build-system change. The change set is exactly four files
(`.gitignore`, `ARCHITECTURE.md`, `CLAUDE.md`, `SPEC.md`); there is no
version bump, so the module still builds as `streamsentry.dll` 0.1.0.
v0.2 itself was approved by the owner on 2026-07-05 after hands-on field
testing of v0.1 (plan and field findings in `reports/V02-PLAN.md`).
Verified by `reports/M5-verifier.md` (VERDICT: VERIFIED, 5/5 checks:
from-scratch configure + build exited 0 with zero compiler/linker warnings,
ctest 2/2 suites — 18/18 cases, artifacts present, source tree untouched)
and `reports/M5-spec-guardian.md` (RESULT: PASS).

#### Added
- `ARCHITECTURE.md`: as-built record of the shipped v0.1 code — module map,
  thread model, fail-closed state machine with its trigger table, data flow,
  render-side plate caching, test infrastructure, and known architectural
  limits. The spec-guardian spot-checked its claims against `src/`
  claim-by-claim and found no materially false claim.
- `SPEC.md` Part 2: the owner-approved v0.2 scope — watcher performance
  hardening (M6), toast geometry narrowing (M6), allowlist mode (M7), panic
  hotkey (M7), and window-picker UI (M8), plus a non-gating Chromium
  password-field investigation — with a v0.2 out-of-scope list and a v0.2
  acceptance-matrix addition table. Nothing in Part 2 exceeds the approved
  plan in `reports/V02-PLAN.md`.

#### Changed
- `SPEC.md`: retitled to cover both versions. Part 1 (v0.1) is preserved as
  the shipped baseline except one inline as-built note: the Win11 toast
  signature (`explorer.exe` + `Xaml_WindowedPopupClass`) supersedes the
  spec's Win10-era `ShellExperienceHost` example.
- `CLAUDE.md`: iron rule 5's scope lock bumped v0.1 → v0.2 — the only
  owner-approved rule change; the other five iron rules are byte-identical.
  The architecture matching line now records the FINAL owner ruling (commit
  a434b18): toast signature = process AND class; block/allowlist matching =
  process image name OR window title (case-insensitive substring) —
  resolving the former CLAUDE-vs-code tension in the ruled direction. The
  summary paragraph now mentions allowlist mode and the panic hotkey.
- `.gitignore`: added a `!ARCHITECTURE.md` whitelist line (the template
  ignores everything not explicitly whitelisted).

### 0.1.0 release candidate (M0-M4)

This Unreleased set (milestones M0 through M4 below) constitutes release
candidate **0.1.0** — ship-ready but not yet published. Publishing and tagging
are a human step (see `reports/FINAL.md`); nothing here has been pushed or
tagged.

- **M4 (docs + packaging, no code logic):** README rewritten to the full
  shipping structure (problem, deterministic/no-AI, fail-closed, opaque-not-blur,
  install, usage, limitations, roadmap, GPL-2.0-or-later); release-zip packaging
  dry-run verified (`streamsentry-0.1.0-windows-x64.zip` layout:
  `streamsentry/bin/64bit/` + `streamsentry/data/`); `reports/FINAL.md` written
  as the consolidated evidence + human-checklist + publish-steps index. Verified
  by `reports/M4-verifier.md` (VERDICT: VERIFIED) and
  `reports/M4-spec-guardian.md` (RESULT: PASS).

### 0.1.0-m3 - 2026-07-05

Milestone M3: the settings UI plus hardening (performance measurement, a
30-minute soak, and an acceptance-matrix mapping). No new detection or masking
behavior. Verified by `reports/M3-verifier.md` (VERDICT: VERIFIED) and
`reports/M3-spec-guardian.md` (RESULT: PASS).

#### Added
- Settings UI (`src/filter.c`): the SPEC "minimal" settings — exactly two
  properties. An **Enable** checkbox (`enabled`, default true) and a multi-line
  **Blocklist** text box (`blocklist`, `OBS_TEXT_MULTILINE`, one process name or
  window-title substring per line), pre-filled with the built-in defaults via
  `ss_watcher_default_blocklist_text()`. There is deliberately **no** option to
  disable fail-closed. Empty text falls back to the built-in defaults.
- Optional performance instrumentation behind the CMake option
  `STREAMSENTRY_PERF_LOG` (`option(... OFF)`, default OFF). When ON it logs the
  watcher tick cost and the render-decision cost; it is compiled out of the
  shipping build (the currently-configured build has `STREAMSENTRY_PERF_LOG=OFF`
  and neither `PERF watcher tick` nor `PERF render decision` string is present in
  the shipping DLL, per M3-verifier Check 2).

#### Changed
- `src/filter.c`: the `blocklist` text is read in `filter_update` and pushed to
  the shared watcher via `ss_watcher_set_blocklist()`. Because there is one
  shared watcher thread, the blocklist is **global** to that thread — each filter
  instance has its own textbox and the last update wins (last-writer-wins). This
  is a documented v0.1 interpretation of the single-watcher architecture
  (inline comment in `filter_update`; surfaced to the human in
  `reports/HUMAN_CHECKLIST.md` item 10).
- `data/locale/en-US.ini`: removed the M2 `DebugGroup` / `DebugKill` strings;
  added `Blocklist` and `BlocklistHint`.

#### Removed
- The developer debug scaffolding is gone from the shipping UI: the M2 developer
  property group and its `debug_kill` "freeze watcher heartbeat" toggle were
  removed from `filter_get_properties`, `filter_get_defaults`, `filter_update`,
  and the `ss_filter` struct. The underlying fault-injection hook
  `ss_watcher_debug_set_killed` (`src/watcher.cpp`) still exists but is wired to
  **no** OBS setting — its only caller is `tests/watcher-selftest.cpp`. Because no
  shipping translation unit references it, the linker's `/OPT:REF` drops the
  unreferenced function and its log string from the release DLL (the DEBUG-kill
  log string is absent from `streamsentry.dll` yet present in
  `watcher-selftest.exe`, per M3-verifier Check 4).

#### Dependencies
- Unchanged from M2. `dumpbin /DEPENDENTS` on both the built and deployed DLL
  returns the identical import set (`obs.dll`, `dwmapi.dll`, `ole32.dll`,
  `USER32.dll`, `w32-pthreads.dll`, `KERNEL32.dll`, `MSVCP140.dll`,
  `VCRUNTIME140_1.dll`, `VCRUNTIME140.dll`, `api-ms-win-crt-*`). No Qt, no
  third-party library. The perf instrumentation uses only `os_gettime_ns` and
  `obs_log` (both already present pre-M3); CMakeLists.txt adds only an `option()`
  plus a `target_compile_definitions` on the existing target.

#### Hardening / performance
Measured on a separate `STREAMSENTRY_PERF_LOG=ON` build (numbers from
`reports/M3-perf.md`, raw soak data in `reports/M3-soak-samples.csv`):
- Watcher thread: ~0.55% of one core (samples 0.803–0.826 ms/tick at the 150 ms
  cadence) — under the SPEC `< 1% of one core` target.
- Render decision cost: avg 0.03 µs/frame (~30 ns), ~0.0002% of a 60 fps
  16 667 µs budget. GPU draw submission is excluded (that is libobs). No
  measurable frame-drop risk.
- 30-minute soak: stable working set with no leak indicators — 345 MB at
  startup, trimmed to ~320–321 MB by ~160 s and flat for the remaining 27 min
  (max 321.1); private bytes ~356.5 MB flat; handles and threads in narrow bands
  (6351–6382 / 234–242) with no monotonic growth. SPEC's endurance target is a
  **2-hour** run; this was 30 min, so the full 2-hour confirmation remains a
  human-checklist item (`reports/HUMAN_CHECKLIST.md` item 9).

### 0.1.0-m2 - 2026-07-05

Milestone M2: real OS-level detection replaces M1's hardcoded fake rects. A
COM MTA watcher thread enumerates windows and reports real screen-space rects,
which the filter maps through the M1 coordinate module. Verified by
`reports/M2-verifier.md` (VERDICT: VERIFIED) and `reports/M2-spec-guardian.md`
(RESULT: PASS).

#### Added
- Detection watcher (`src/watcher.cpp`): one COM MTA thread
  (`CoInitializeEx(nullptr, COINIT_MULTITHREADED)`), refcounted so multiple
  filter instances share the single thread. It enumerates windows on a 150 ms
  timer (`WATCH_TICK_MS`) and publishes screen-space rects plus a heartbeat
  timestamp into shared state each live tick.
- Toast detection: matches on process name AND window class. The signature was
  determined empirically on this machine as `explorer.exe` +
  `Xaml_WindowedPopupClass` (Windows 11 build 26200); this supersedes SPEC's
  Win10-era `ShellExperienceHost` example, and the determination is recorded in
  code comments. Candidates are gated by DWMWA_CLOAKED (cloaked skipped), plus
  visible and non-empty rect. The class also matches other XAML flyouts (Start
  search, taskbar popups), which are therefore over-masked — accepted under
  iron rule 1 and documented as a known limitation.
- Blocklist detection: matches when the process image name OR the window-title
  substring matches an entry (both case-insensitive). Built-in defaults are
  password managers and credential dialogs (1Password, KeePass, Bitwarden,
  Dashlane, LastPass, `consent.exe`, `logonui`, `credentialuibroker`). Wiring a
  user-editable blocklist textbox is deferred to M3; the running plugin uses the
  built-in defaults (`ss_watcher_set_blocklist` exists and is exercised by the
  self-test but is not yet wired to filter settings).
- Password-field detection: a UIA focus-changed handler reads
  `UIA_IsPasswordProperty` and, on a password element, copies
  `BoundingRectangle` under a small lock and returns. The handler does minimal
  work (no enumeration, no blocking); enumeration runs on the timer.
- Filter-side capture-geometry resolution (`src/geom-resolve.c`): resolves
  geometry only for display/monitor capture, and only when unambiguous — exactly
  one monitor whose native pixel size equals the source base size. Any other
  source id, a zero base size, zero monitors, or more than one same-size monitor
  returns false and the caller fails closed. It does not trust an OBS monitor
  index/id, because a same-resolution neighbour would give the wrong origin
  (under-mask). Dual identical-resolution monitors are a documented v0.1
  limitation.

#### Changed
- `src/filter.c`: rewired to map the watcher's real rects through the M1
  coordinate module instead of injecting fake rects. Startup is fail-closed
  (black) until the watcher's first heartbeat proves detection is alive
  (observed in OBS as FAIL-CLOSED at load, then cleared ~140 ms later once the
  heartbeat was fresh). M1's fake-rect / simulate-stall DEBUG toggles were
  replaced by a single `debug_kill` toggle that freezes the watcher heartbeat to
  force fail-closed; it feeds no path that can weaken or disable the blackout,
  and `filter_destroy` releases a lingering kill so a fault-injecting instance
  cannot leave the shared watcher frozen for others.
- `src/shared-state.h`: slimmed for M2 — the geometry fields were removed; the
  watcher emits screen-space rects only and the filter resolves its own capture
  geometry per frame.
- `data/locale/en-US.ini`: removed the M1 `DebugRects` / `DebugStall` strings;
  added a single `DebugKill` = "DEBUG: freeze watcher heartbeat (forces
  fail-closed blackout)".
- `CMakeLists.txt`: added `src/watcher.cpp`, `src/geom-resolve.c`, and the new
  headers to the module target, and a `watcher-selftest` executable
  (`tests/watcher-selftest.cpp`, intentionally not registered with ctest because
  it has side effects).

#### Dependencies
- New Windows SDK link/import libraries: `dwmapi` (DWMWA_CLOAKED), `ole32`
  (COM init/create), `oleaut32`, `uuid` (static GUIDs), and `user32`
  (`EnumWindows` / window-class and title queries). `dwmapi.dll`, `ole32.dll`,
  and `USER32.dll` appear as new runtime imports (`oleaut32`/`uuid` link but do
  not add runtime imports).
- The MSVC C++ runtime (`MSVCP140.dll`, `VCRUNTIME140_1.dll`) is now imported,
  introduced by `watcher.cpp` using the C++ standard library
  (`std::mutex`/`vector`/`wstring`). No third-party libraries, no Qt.

#### Decisions
- Blocklist matching uses process-name-OR-title-substring (per SPEC Detection §2
  and iron rule 1's over-mask requirement); CLAUDE.md's "process AND class" is
  applied to toasts only. Requiring both process AND an exact class for the
  blocklist would AND two failure modes and make a miss (under-mask) more likely,
  which iron rule 1 forbids. This is documented in `reports/M2-DECISIONS.md` and
  flagged for the owner's ruling (see `reports/HUMAN_CHECKLIST.md`); it is a
  one-line change if strict AND-semantics are mandated instead.

### 0.1.0-m1 - 2026-07-05

Milestone M1: mask-render pipeline driven by hardcoded fake rects (real
detection is M2). Verified by `reports/M1-verifier.md` (VERDICT: VERIFIED)
and `reports/M1-spec-guardian.md` (RESULT: PASS).

#### Added
- Pure coordinate-mapping module (`src/coord-map.c`): maps screen coordinates
  to capture-source coordinates, handling multi-monitor offsets and negative
  origins, per-capture mixed-DPI ratios, and scaled/cropped captures. Applies
  over-mask padding on all four sides and clamps it to source bounds so padding
  cannot push a mask off-source. Returns `SS_MAP_INVALID` on bad geometry
  (NULL/non-finite/zero/negative inputs) so the caller fails closed;
  `SS_MAP_NOT_VISIBLE` is reserved for genuine off-capture rects. No OBS or
  Windows includes (only `math.h`). Covered by 11 ctest cases.
- Pure plate generator (`src/plate-gen.c`): opaque notification card (rounded
  rect + bell icon + "Notification hidden") for toast rects, opaque privacy
  plate (dark fill + lock icon + "Hidden") for window/field rects, and an
  opaque fail-closed status banner. Opacity is pixel-asserted in ctest (every
  pixel inside the corner inset has alpha == 255; banner fully opaque at inset
  0; corner inset 5 < pad 12). Uses an original hand-drawn 5x7 bitmap font; no
  third-party assets.
- Shared state (`src/shared-state.c`): rect list plus heartbeat timestamp, with
  a non-blocking `trylock`-based reader (returns false on contention rather than
  blocking).
- Filter render path (`src/filter.c`): maps the fake rects to plates when
  healthy; draws full opaque black plus the status banner when unhealthy. The
  six fail-closed triggers are heartbeat older than 500 ms, missing snapshot,
  invalid geometry, mapping failure, filter-begin bypass, and texture-allocation
  failure. The 500 ms threshold is a compile-time constant
  (`SS_HEARTBEAT_STALE_NS`), not user-settable. Fail-closed was measured at
  ~517 ms in OBS (log delta; internal heartbeat age 519 ms).
- Two developer-only DEBUG toggles (inject fake rects, simulate watcher stall)
  as M1 scaffolding to be removed at M3. Neither is read by the render/health
  path, so they can only trigger fail-closed or inject test rects — they cannot
  disable it.

#### Changed
- `src/plugin-main.c`: removed the M0 inline pass-through filter; now registers
  the filter from `src/filter.c` and initializes/frees shared state in
  `obs_module_load`/`obs_module_unload`.
- `CMakeLists.txt`: added the new first-party `src/` files to the module target
  and a `STREAMSENTRY_BUILD_TESTS` option that builds two standalone ctest
  binaries (`coord-map-tests`, `plate-gen-tests`) from the test file plus the
  pure module only (they link no libobs and ship nothing into the plugin).

#### Dependencies
- New shipped-DLL import `w32-pthreads.dll` — libobs's own pthread shim (pulled
  transitively via `util/threading.h`), not a third-party library. Also a new
  `api-ms-win-crt-math` import from `coord-map.c`'s use of `math.h`. No Qt, no
  other new DLLs.

### Renamed - 2026-07-05

Plugin renamed from `obsplugin` to **StreamSentry** (owner's choice, no
behavior change):
- `buildspec.json`: `name` → `streamsentry`, `displayName` → `StreamSentry`,
  macOS `bundleId` → `com.example.streamsentry`. `CMAKE_PROJECT_NAME` (and
  therefore `PLUGIN_NAME` and the built DLL filename) follow `name`
  automatically, so the artifact is now `streamsentry.dll`.
- Filter source id `obsplugin_filter` → `streamsentry_filter`; struct/function
  prefixes renamed to match.
- Localized display string (`data/locale/en-US.ini`) `FilterName` →
  `StreamSentry`.
- Re-verified after rename: build green (`streamsentry.dll`, 14,848 bytes),
  `dumpbin /dependents` still libobs-only (no Qt), `obs_module_load` exported,
  redeployed to `D:\software\obs\obs-studio`, OBS log confirms
  `[streamsentry] plugin loaded successfully (version 0.1.0)`.
- The repository directory itself could not be renamed from `F:\obsplugin`
  within the Claude Code session that did this work — the harness protects
  its own primary working directory from removal/rename. A full mirror
  (all git history, `.deps`, working tree) was copied via `robocopy` to
  `F:\StreamSentry`; that copy is the canonical one to use going forward.
  `F:\obsplugin` should be deleted manually (outside that session) once the
  owner has confirmed `F:\StreamSentry` works.

### 0.1.0-m0 - 2026-07-05

Milestone M0: project skeleton. Verified by `reports/M0-verifier.md`
(VERDICT: VERIFIED) and `reports/M0-spec-guardian.md` (RESULT: PASS).

- Initialized the plugin skeleton from obs-plugintemplate (upstream commit
  3e7d7ac, per project owner's record; local import commit eb47d8b).
- Plugin identity set in `buildspec.json`: name/display name `obsplugin`,
  version `0.1.0`.
- Extended the template's whitelist-style `.gitignore` to track project
  governance files (CLAUDE.md, SPEC.md, SETUP.md, TESTING.md, CHANGELOG.md,
  `.claude/`, `scripts/`, `reports/`).
- Registered a minimal pass-through video filter (source id
  `obsplugin_filter`, display name "obsplugin"): one "Enable" checkbox
  (default checked); the render callback skips the filter, so video output
  is unmodified. No detection, no masking yet.
- Template CI retained unmodified (`.github/` byte-identical to the import
  commit): pushing a tag triggers a build and a draft GitHub release
  (`.github/workflows/push.yaml`).
- Local builds use an untracked, git-ignored `CMakeUserPresets.json`
  (preset `windows-x64-local`) to select the locally installed Windows SDK
  instead of the template-pinned 10.0.22621; it does not affect CI.
