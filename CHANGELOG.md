# Changelog

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## Unreleased

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
