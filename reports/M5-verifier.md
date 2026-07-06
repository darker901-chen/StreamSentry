# M5 Verifier Report — StreamSentry v0.2 kickoff (docs + governance)

- Date: 2026-07-06
- Platform: Windows 11 (10.0.26200), x64
- Repo: F:\obsplugin  (branch: master, HEAD 0417d70)
- Milestone: M5 = docs+governance only — ARCHITECTURE.md added, SPEC.md v0.2 Part 2
  appended, CLAUDE.md scope bump, .gitignore whitelist. NO changes under src/,
  tests/, or CMakeLists.txt. Task: prove the tree still builds green and all
  automated tests pass after these doc changes.
- Configure under test: preset `windows-x64-local` (untracked CMakeUserPresets.json,
  inherits windows-x64; build config RelWithDebInfo per the build preset).
- Toolchain observed at configure: Windows SDK 10.0.26100.0, MSVC 19.44.35224.0
  (cl at C:/BuildTools/VC/Tools/MSVC/14.44.35207), CMake 3.28.0-rc5,
  OBS sources dependency: 31.1.1 (Build 15). OBS was NOT launched.

## FINAL VERDICT: VERIFIED (5 / 5 checks PASS)

---

## Commands executed (verbatim, in order)

    git status --short                                   (before; and again after build)
    rm -rf F:/obsplugin/build_x64                        (true clean: build dir deleted)
    cmake --preset windows-x64-local                     -> exit 0
    cmake --build --preset windows-x64-local             -> exit 0  (RelWithDebInfo)
    ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure   -> exit 0
    F:\obsplugin\build_x64\RelWithDebInfo\coord-map-tests.exe          -> exit 0
    F:\obsplugin\build_x64\RelWithDebInfo\plate-gen-tests.exe          -> exit 0
    ls -la F:/obsplugin/build_x64/RelWithDebInfo/        (artifact listing)

## Check 1 — git-tracked source untouched; only expected M5 doc changes — PASS

`git status --short` (identical before AND after the clean build):

    M  .gitignore
    A  ARCHITECTURE.md
    M  CLAUDE.md
    M  SPEC.md

Exactly the four expected files, ALL STAGED, nothing unstaged/untracked. No entry
for src/, tests/, or CMakeLists.txt — git-tracked source has no staged or unstaged
modifications. build_x64/, .deps/, CMakeUserPresets.json remain ignored.

## Check 2 — Clean configure — PASS (exit 0)

build_x64 was DELETED first, so this is a from-scratch configure, not incremental.
`cmake --preset windows-x64-local` exit code: 0. Log tail:

    -- Configuring done (29.1s)
    -- Generating done (0.0s)
    -- Build files have been written to: F:/obsplugin/build_x64

Cache confirmation (build_x64/CMakeCache.txt):

    CMAKE_CONFIGURATION_TYPES:STRING=Debug;Release;MinSizeRel;RelWithDebInfo
    STREAMSENTRY_PERF_LOG:BOOL=OFF

Accepted warnings (listed, not ignored) — both originate in the OBS-SOURCES
DEPENDENCY configure step (.deps obs-studio 31.1.1), NOT in plugin code, and are
pre-existing template/dependency noise:

    CMake Warning (dev) at cmake/finders/FindDetours.cmake:65: Failed to find
      detours version.  (call stack: plugins/win-capture/graphics-hook)
    CMake Warning at plugins/win-dshow/virtualcam-module/CMakeLists.txt:14:
      Empty Virtual Camera GUID set.

Benign probe lines: CMAKE_HAVE_LIBC_PTHREAD Failed / pthread_create not found,
then "Found Threads: TRUE" — normal CMake Threads detection on MSVC.

## Check 3 — Clean build, RelWithDebInfo, zero warnings — PASS (exit 0)

`cmake --build --preset windows-x64-local` exit code: 0. Because the build dir was
fresh, EVERY translation unit compiled. Output (MSBuild 17.14.40; linker/codegen
status lines localized zh-TW):

    plugin-support.c -> plugin-support.lib
    plugin-main.c / filter.c / coord-map.c / plate-gen.c / shared-state.c /
    geom-resolve.c / watcher.cpp
    streamsentry.vcxproj -> F:\obsplugin\build_x64\RelWithDebInfo\streamsentry.dll
    coord-map-tests.vcxproj -> ...\coord-map-tests.exe
    plate-gen-tests.vcxproj -> ...\plate-gen-tests.exe
    watcher-selftest.vcxproj -> ...\watcher-selftest.exe

Warning scan of the full build log: regex `warning|error|fatal|C4\d{3}|C5\d{3}|LNK\d+`
(case-insensitive) — NO matches; localized scan `警告|錯誤|错误` — NO matches.
ZERO compiler/linker warnings.

Honesty note: unlike the M4 run (where CMAKE_COMPILE_WARNING_AS_ERROR=ON was passed
manually and appeared UNINITIALIZED in cache), this preset-only clean configure does
NOT set warnings-as-error — the cache has no CMAKE_COMPILE_WARNING_AS_ERROR entry.
The zero-warning claim above therefore rests on the log grep of a full clean
recompile, which found nothing. Same outcome, different mechanism.

## Check 4 — Automated tests — PASS (2/2 suites, 18/18 cases)

    ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure
    1/2 Test #1: coord-map-tests ..................   Passed    0.02 sec
    2/2 Test #2: plate-gen-tests ..................   Passed    0.14 sec
    100% tests passed, 0 tests failed out of 2
Exit code: 0.

Per-suite case detail (executables run directly; each main() unconditionally runs
every case and returns nonzero on any failure, so "all passed" + exit 0 = all cases):

- coord-map-tests: 11/11 PASS ("coord-map-tests: all passed", exit 0) — identity,
  secondary_monitor_offset, negative_origin_monitor, mixed_dpi, scaled_capture,
  partial_overlap_cropped, not_visible, padding_expansion, padding_clamped_at_edges,
  padding_never_shrinks, invalid_inputs. (These are the coordinate-mapping unit tests.)
- plate-gen-tests: 7/7 PASS ("plate-gen-tests: all passed", exit 0) —
  toast_card_opacity, toast_card_corners_transparent, privacy_plate_opacity,
  status_banner_fully_opaque, no_rgb_bleed_means_text_present,
  degenerate_sizes_do_not_crash, padding_exceeds_corner_inset.

## Check 5 — Binary artifacts present — PASS

build_x64\RelWithDebInfo\ (all freshly built this run, timestamps 2026-07-06):

    streamsentry.dll        64000 bytes   (same size as the M4-verified DLL)
    streamsentry.pdb      1536000 bytes
    coord-map-tests.exe     21504 bytes  (+ .pdb)
    plate-gen-tests.exe     19456 bytes  (+ .pdb)
    watcher-selftest.exe    50688 bytes  (+ .pdb)  — BUILT ONLY, NOT RUN (see below)

Required streamsentry.dll + streamsentry.pdb both exist at the expected location.

---

## Not covered by automation (stated explicitly)

- watcher-selftest.exe is manual-only and intentionally NOT registered with ctest
  (it launches notepad.exe and fires a real toast). Per instruction it was NOT run
  this milestone; verification here is limited to "it still compiles and links".
- In-OBS toast masking behavior requires the manual test matrix in SPEC.md
  (real toast with DND off, blocklist app, password-field focus) — not automated.
- In-OBS visual confirmation of plate/card rendering and fail-closed blackout
  (OBS was not launched).
- Live multi-monitor / mixed-DPI coordinate accuracy on real hardware (unit tests
  cover the math only).
- Endurance soak and performance are unchanged claims from M3; not re-measured.
- CONTENT of the M5 documents (ARCHITECTURE.md accuracy, SPEC.md v0.2 Part 2
  consistency, CLAUDE.md scope wording) is NOT judged by this build verification —
  that is spec-guardian territory. This report only proves the tree builds and
  tests green with those files present.

## FINAL VERDICT: VERIFIED
