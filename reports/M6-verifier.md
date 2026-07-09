# M6 Verifier Report — StreamSentry v0.2 hardening (perf cache + toast geometry gate)

- Date: 2026-07-09 (run 1 ~21:04–21:10; run 2 ~21:15–21:19 — see ADDENDUM)
- Platform: Windows 11 (10.0.26200), x64
- Repo: F:\obsplugin  (branch: master, HEAD 6667ae5 + staged M6 change set, see Check 1)
- Milestone: M6 = SPEC Part 2 items 2.1 + 2.2. Staged diff under test:
  PID→image-name cache, always-on >250ms tick warning, upgraded PERF_LOG stats
  (avg/max/p99 + cache hit rate) in src/watcher.cpp; new pure module
  src/toast-gate.c/.h wired into the toast signature match; new ctest suite
  tests/toast-gate-tests.c; CMakeLists.txt wiring (plugin sources, test target,
  toast-gate.c in watcher-selftest); ARCHITECTURE.md update; reports/M6-toast-probe.txt.
- Configure under test: preset `windows-x64-local` (CMakeUserPresets.json, inherits
  windows-x64; build dir build_x64; RelWithDebInfo). Default STREAMSENTRY_PERF_LOG=OFF
  is the configuration under test; the ON variant was compile-verified additionally.
- Toolchain observed at configure: Windows SDK 10.0.26100.0 (targeting 10.0.26200),
  MSVC 19.44.35224.0 (cl at C:/BuildTools/VC/Tools/MSVC/14.44.35207),
  CMake 3.28.0-rc5, MSBuild 17.14.40, OBS sources dependency 31.1.1 (Build 23).
  OBS itself was NOT launched.

## FINAL VERDICT: VERIFIED — 7 / 7 checks PASS in run 1 AND in the run-2 clean re-verification of the amended tree (the ADDENDUM is authoritative for the final staged tree)

---

## Commands executed (verbatim, in order)

    git status --short && git log --oneline -3
    git diff --cached --stat
    rm -rf /f/obsplugin/build_x64                                  (true clean)
    cmake --preset windows-x64-local                                -> exit 0
    cmake --build --preset windows-x64-local                        -> exit 0  (full clean build, OFF)
    cmake --preset windows-x64-local -DSTREAMSENTRY_PERF_LOG=ON     -> exit 0
    cmake --build --preset windows-x64-local                        -> exit 0  (ON variant)
    cmake --preset windows-x64-local -DSTREAMSENTRY_PERF_LOG=OFF    -> exit 0
    cmake --build --preset windows-x64-local                        -> exit 0  (back to OFF; final tree state)
    ctest --test-dir /f/obsplugin/build_x64 -C RelWithDebInfo --output-on-failure   -> exit 0
    ./coord-map-tests.exe; ./plate-gen-tests.exe; ./toast-gate-tests.exe            -> exit 0 each
    ./watcher-selftest.exe                                          -> exit 0 (full output below)
    ls -la /f/obsplugin/build_x64/RelWithDebInfo/
    ls -la /d/software/obs/obs-studio/obs-plugins/64bit/streamsentry.dll
    git -C /f/obsplugin status --short                              (after; unchanged)

## Check 1 — Tree state (run 1): exactly the staged M6 change set at run-1 start and end — PASS (tree amended at 21:13, see ADDENDUM)

`git status --short` identical at run-1 start and end (nothing modified
outside reports/ by this verification):

    M  ARCHITECTURE.md
    M  CMakeLists.txt
    A  reports/M6-toast-probe.txt
    A  src/toast-gate.c
    A  src/toast-gate.h
    M  src/watcher.cpp
    A  tests/toast-gate-tests.c

`git diff --cached --stat`: 7 files, 642 insertions(+), 45 deletions(-). Matches the
milestone description; coord-map/plate-gen sources are NOT touched this milestone.

## Check 2 — Clean configure, default cache (PERF_LOG=OFF) — PASS (exit 0)

build_x64 was DELETED first: from-scratch configure, not incremental.
"Configuring done (23.1s)"; "Build files have been written to: F:/obsplugin/build_x64".
Cache confirmation (build_x64/CMakeCache.txt):

    CMAKE_CONFIGURATION_TYPES:STRING=Debug;Release;MinSizeRel;RelWithDebInfo
    STREAMSENTRY_PERF_LOG:BOOL=OFF

Accepted warnings (listed, not ignored) — both from the OBS-SOURCES DEPENDENCY
configure step (.deps obs-studio 31.1.1), pre-existing template/dependency noise,
identical to M4/M5 runs, NOT from plugin code:

    CMake Warning (dev) at cmake/finders/FindDetours.cmake:65 (message):
      Failed to find detours version.
      (call stack: plugins/win-capture/graphics-hook/CMakeLists.txt:3)
    CMake Warning at plugins/win-dshow/virtualcam-module/CMakeLists.txt:14 (message):
      Empty Virtual Camera GUID set.

Benign probe lines: CMAKE_HAVE_LIBC_PTHREAD Failed / pthread_create not found, then
"Found Threads: TRUE" — normal CMake Threads detection on MSVC.

## Check 3 — Full clean build, RelWithDebInfo, PERF_LOG=OFF, zero warnings — PASS (exit 0)

Fresh build dir, so EVERY translation unit compiled (MSBuild 17.14.40; codegen status
lines localized zh-TW). Compile roster from the log confirms the new wiring:
toast-gate.c compiled 3x (streamsentry plugin + toast-gate-tests + watcher-selftest),
watcher.cpp 2x (plugin + selftest). All six targets produced:

    plugin-support.vcxproj   -> build_x64\RelWithDebInfo\plugin-support.lib
    streamsentry.vcxproj     -> build_x64\RelWithDebInfo\streamsentry.dll
    coord-map-tests.vcxproj  -> build_x64\RelWithDebInfo\coord-map-tests.exe
    plate-gen-tests.vcxproj  -> build_x64\RelWithDebInfo\plate-gen-tests.exe
    toast-gate-tests.vcxproj -> build_x64\RelWithDebInfo\toast-gate-tests.exe
    watcher-selftest.vcxproj -> build_x64\RelWithDebInfo\watcher-selftest.exe

Warning scan of the full build log: case-insensitive regex
warning|error|fatal|C4[0-9]{3}|C5[0-9]{3}|LNK[0-9]+  -> 0 matches;
localized scan for the zh-TW warning/error terms -> 0 matches.
ZERO compiler/linker warnings.

Honesty note (same as M5): this preset does not set CMAKE_COMPILE_WARNING_AS_ERROR;
the zero-warning claim rests on the log grep of a full clean recompile.

## Check 4 — PERF_LOG=ON variant compiles; tree returned to OFF — PASS

1. cmake --preset windows-x64-local -DSTREAMSENTRY_PERF_LOG=ON -> exit 0;
   cache verified STREAMSENTRY_PERF_LOG:BOOL=ON BEFORE building.
2. Build -> exit 0. The definition change dirtied the whole plugin target: all 8
   streamsentry TUs recompiled (plugin-main.c, filter.c, coord-map.c, plate-gen.c,
   shared-state.c, geom-resolve.c, toast-gate.c, watcher.cpp). Warning scan (same
   EN + zh-TW regexes): 0 matches. ON variant compiles clean.
3. cmake --preset windows-x64-local -DSTREAMSENTRY_PERF_LOG=OFF -> exit 0;
   cache verified back to STREAMSENTRY_PERF_LOG:BOOL=OFF.
4. Rebuild -> exit 0; full plugin target recompile again; warning scans 0 matches.
   The tree is LEFT IN THE OFF STATE (cache OFF; final streamsentry.dll is the OFF
   build, timestamp 21:08). Note: the ON-variant DLL was overwritten by this final
   OFF rebuild by design; the ON evidence is the build log + cache line above.

## Check 5 — Automated tests — PASS (3/3 suites)

    ctest --test-dir /f/obsplugin/build_x64 -C RelWithDebInfo --output-on-failure
    1/3 Test #1: coord-map-tests ..................   Passed    0.02 sec
    2/3 Test #2: plate-gen-tests ..................   Passed    0.14 sec
    3/3 Test #3: toast-gate-tests .................   Passed    0.02 sec
    100% tests passed, 0 tests failed out of 3
Exit code: 0. Exactly the 3 expected suites — toast-gate-tests is newly registered.

Direct execution (each main() unconditionally runs every case and returns nonzero on
any failure, so "all passed" + exit 0 = all cases):

- coord-map-tests.exe:  "coord-map-tests: all passed", exit 0 (11 cases per M5 report;
  source unchanged this milestone).
- plate-gen-tests.exe:  "plate-gen-tests: all passed", exit 0 (7 cases per M5 report;
  source unchanged this milestone).
- toast-gate-tests.exe: "toast-gate-tests: all passed", exit 0 — 23 CHECK assertions
  read from tests/toast-gate-tests.c: 6 positive toast shapes (documented metrics at
  100/150/200% DPI, slide-in animation, negative-origin secondary monitor), 10
  negative/flyover fixtures from reports/M6-toast-probe.txt (incl. the tray-overflow
  shape asserted as ADMITTED by design — over-mask-safe, so future tightening is a
  conscious act), 2 multi-monitor, 5 uncertainty-fails-toward-masking (NULL/zero
  monitors, degenerate, NaN).

## Check 6 — watcher-selftest.exe (milestone convention) — PASS (exit 0, 8/8 deterministic)

Full output, verbatim:

    ok:   watcher thread started
    watcher thread started (tick 150ms)
    ok:   heartbeat advances while watcher alive
    info: baseline blocklist rects = 0
    ok:   launched notepad.exe
    ok:   blocklist rect appeared after notepad opened
    ok:   blocklist rect disappeared after notepad closed
    watcher: DEBUG kill engaged (heartbeat frozen)
    ok:   heartbeat frozen while killed (>500ms) -> render fails closed
    watcher: DEBUG kill released
    ok:   heartbeat resumes after un-kill
    INCONCLUSIVE: no toast banner detected (likely Do Not Disturb / focus assist suppressing the banner) - see reports/M2-DECISIONS.md; real toast masking is on the human checklist
    watcher thread stopped
    ok:   watcher thread stopped cleanly
    watcher-selftest: all deterministic checks passed

All 8 deterministic checks pass. The toast leg is INCONCLUSIVE — EXPECTED and
documented for this machine: toast banners are currently system-suppressed (every
notification goes silently to Notification Center; diagnosis + owner ruling in
reports/M6-toast-probe.txt, 2026-07-09). This is a machine-state limitation, not a
regression: the selftest treats the toast probe as best-effort by design and no toast
banner window exists to detect. Side effect as designed: the selftest launches and
then force-kills all notepad.exe instances (baseline was 0).

## Check 7 — Binary artifacts present — PASS

build_x64\RelWithDebInfo\ (all built this run, 2026-07-09 21:06/21:08):

    streamsentry.dll        70144 bytes  21:08  (final OFF rebuild)
    streamsentry.pdb      1757184 bytes  21:08
    toast-gate-tests.exe    16384 bytes  21:06  (+ .pdb)
    coord-map-tests.exe     21504 bytes  21:06  (+ .pdb)
    plate-gen-tests.exe     19456 bytes  21:06  (+ .pdb)
    watcher-selftest.exe    57344 bytes  21:06  (+ .pdb; w32-pthreads.dll copied beside)

Required streamsentry.dll + streamsentry.pdb and toast-gate-tests.exe all exist at the
expected location.

Deployment note (informational, per task — NOT a pass/fail item):
D:\software\obs\obs-studio\obs-plugins\64bit\streamsentry.dll exists, 72704 bytes,
mtime today 2026-07-09 21:00 (minutes before this clean run) — consistent with the
perf-instrumented (PERF_LOG=ON) build deployed for the owner soak measurement
(size differs from the fresh OFF DLL, 72704 vs 70144). Provenance/config of that
deployed binary was not independently verified by this run.

---

## Not covered by automation (stated explicitly)

- SPEC 2.1 acceptance — 30-min streaming soak, busy desktop, zero stale-heartbeat
  fail-closed events, measured tick p99 <= 50ms recorded in reports/: REQUIRES the
  owner manual soak with the perf-instrumented deployed build. Nothing in this run
  measures tick latency under load; PERF_LOG stats (avg/max/p99, cache hit rate) are
  verified COMPILE-ONLY here (both ON and OFF variants build; no runtime perf sample
  was produced or judged).
- SPEC 2.2 acceptance — real-toast masking (toast masked before content readable) and
  the no-plate-storm rows (Start search / taskbar flyouts / tray overflow): REQUIRE
  the owner manual pass in OBS. On this machine toast banners are currently
  system-suppressed (reports/M6-toast-probe.txt), so no local run — automated or
  manual — can currently exercise a real banner; the selftest toast leg is
  INCONCLUSIVE accordingly. The unit suite covers the geometry gate math against
  documented metrics and recorded negative fixtures only.
- The toast-gate constants are PROVISIONAL by owner ruling (documented-values basis,
  generous over-mask-safe bounds); empirical calibration and re-verification of the
  v0.1 toast signature on Windows build 26200.8655 are deferred to the owner
  hands-on acceptance once banners display again (ruling recorded in
  reports/M6-toast-probe.txt).
- The always-on >250ms single-tick warning compiles but was never triggered in any
  automated test (no slow-tick fault injection exists for it). PID-cache behavior
  under real process churn / PID reuse is exercised only incidentally (notepad
  launch/kill in the selftest), not asserted.
- In-OBS behavior (plate/card rendering, fail-closed blackout visuals, toast card on
  a real banner): OBS was not launched.
- Live multi-monitor / mixed-DPI accuracy on real hardware: unit tests cover the math
  only.
- CONTENT correctness of the ARCHITECTURE.md changes and of the M6-toast-probe.txt
  record is spec-guardian territory, not judged by this build verification.

## RUN-1 VERDICT: VERIFIED (tree as of 21:04–21:09 — superseded by the ADDENDUM for the final tree)

---

# ADDENDUM — mid-verification tree amendment (21:13) and clean re-verification (run 2, authoritative)

## What happened

At 21:12–21:13, AFTER run-1 verification had completed (~21:10) and while this
report was being written, a concurrent M6 agent amended src/watcher.cpp and staged
the change; reports/M6-spec-guardian.md also appeared (untracked, 21:12). The staged
diff grew from 642 to 656 insertions (watcher.cpp 180 -> 194). The +14-line delta
(read from the diff before it was staged): an EnumCtx::mons_complete flag — if
EnumDisplayMonitors fails, truncates (>16 monitors), or GetMonitorInfoW fails for
any monitor, the monitor list handed to the toast gate is emptied, so the gate
classifies every signature match as a toast (over-mask, iron-rule-1 direction,
instead of a toast on an unlisted monitor failing the overlap test and going
unmasked).

Run-1 binaries were compiled from the PRE-amendment watcher.cpp. Additionally, an
external unlogged rebuild touched build_x64 at 21:13 (watcher.obj timestamps), so no
evidence I witnessed covered the amended file. Verdict for the CURRENT tree therefore
required a full clean re-run: run 2.

## Tree pinning (no further drift)

    git ls-files -s src/watcher.cpp   -> 100644 2f2e6f7845d065ce51820244c1fb5425b344d934 0
    git hash-object src/watcher.cpp   -> 2f2e6f7845d065ce51820244c1fb5425b344d934

Index blob == worktree blob, verified immediately BEFORE run 2 and again AFTER run 2:
unchanged. Staged set after run 2: the same 7 files, 656 insertions(+), 45
deletions(-); no unstaged changes; untracked files are only the two M6 reports.

## Run 2 — commands (verbatim) and results

    rm -rf /f/obsplugin/build_x64                                   (true clean, again)
    cmake --preset windows-x64-local                                -> exit 0; cache STREAMSENTRY_PERF_LOG:BOOL=OFF
    cmake --build --preset windows-x64-local                        -> exit 0  (full clean build, OFF)
    cmake --preset windows-x64-local -DSTREAMSENTRY_PERF_LOG=ON     -> exit 0; cache ON (verified before build)
    cmake --build --preset windows-x64-local                        -> exit 0  (ON variant; all 8 plugin TUs recompiled)
    cmake --preset windows-x64-local -DSTREAMSENTRY_PERF_LOG=OFF    -> exit 0; cache OFF (verified)
    cmake --build --preset windows-x64-local                        -> exit 0  (all 8 plugin TUs recompiled; tree LEFT OFF)
    ctest --test-dir /f/obsplugin/build_x64 -C RelWithDebInfo --output-on-failure  -> exit 0
    ./coord-map-tests.exe; ./plate-gen-tests.exe; ./toast-gate-tests.exe           -> exit 0 each, "all passed"
    ./watcher-selftest.exe                                          -> exit 0

Warning scans on ALL THREE run-2 build logs (clean OFF, ON, OFF-return): the same
EN regex (warning|error|fatal|C4[0-9]{3}|C5[0-9]{3}|LNK[0-9]+, case-insensitive) and
zh-TW terms -> 0 matches everywhere. The clean OFF build compiled the full 19-TU
roster (toast-gate.c x3, watcher.cpp x2 — plugin + selftest) and produced all six
targets, exactly as in run 1 / Check 3.

Run-2 ctest:

    1/3 Test #1: coord-map-tests ..................   Passed    0.02 sec
    2/3 Test #2: plate-gen-tests ..................   Passed    0.14 sec
    3/3 Test #3: toast-gate-tests .................   Passed    0.02 sec
    100% tests passed, 0 tests failed out of 3

Run-2 watcher-selftest.exe output: line-for-line identical to the run-1 transcript in
Check 6 — all 8 deterministic checks ok, toast leg INCONCLUSIVE (expected, banners
system-suppressed per reports/M6-toast-probe.txt), exit 0,
"watcher-selftest: all deterministic checks passed".

Run-2 artifacts (build_x64\RelWithDebInfo\, 2026-07-09 21:17/21:18): streamsentry.dll
70144 bytes + streamsentry.pdb 1757184 bytes (21:18, OFF build);
toast-gate-tests.exe 16384; coord-map-tests.exe 21504; plate-gen-tests.exe 19456;
watcher-selftest.exe 57344; w32-pthreads.dll copied beside. All required artifacts
present.

## Addendum to coverage gaps

Everything in the run-1 "Not covered by automation" section applies unchanged. One
addition for the 21:13 amendment: the empty-monitor-list fallback is covered at the
toast-gate level by the existing uncertainty fixtures (NULL/zero monitors -> classify
as toast), but the watcher-side TRIGGERS (real EnumDisplayMonitors failure, >16
monitors, GetMonitorInfoW failure) have no automated fixture — the fallback wiring
inside watcher.cpp is verified by inspection and compilation only.

## FINAL VERDICT (authoritative, for the staged tree with watcher.cpp blob 2f2e6f7): VERIFIED
