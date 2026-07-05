# Changelog

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## Unreleased

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
