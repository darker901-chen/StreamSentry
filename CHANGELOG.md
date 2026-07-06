# Changelog

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## Unreleased

Two unreleased sets live here, newest first: the v0.2 development cycle
(M5 onward) and, below it, the 0.1.0 release-candidate set (M0 through M4).
Nothing has been pushed or tagged; publishing is a human step.

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
