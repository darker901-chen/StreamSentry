# M4 Verifier Report — StreamSentry v0.1 (ship-ready)

- Date: 2026-07-05
- Platform: Windows 11, x64
- Repo: F:\obsplugin  (branch: master)
- Milestone: M4 = final docs + release packaging; NO new code logic
- Configure under test: preset `windows-x64-local` (untracked CMakeUserPresets.json),
  cache confirms `CMAKE_COMPILE_WARNING_AS_ERROR=ON`, `STREAMSENTRY_PERF_LOG=OFF`
  (CI-parity strictness).
- dumpbin: C:\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe
- OBS was NOT launched.

## FINAL VERDICT: VERIFIED (7 / 7 checks PASS)

---

## Check 1 — CI-parity build (warnings-as-error), exit 0, ZERO warnings — PASS

Cache confirmation (build_x64/CMakeCache.txt):
    CMAKE_COMPILE_WARNING_AS_ERROR:UNINITIALIZED=ON
    STREAMSENTRY_PERF_LOG:BOOL=OFF
    CMAKE_CONFIGURATION_TYPES:STRING=Debug;Release;MinSizeRel;RelWithDebInfo

Command (incremental, as CI would invoke):
    cmake --build --preset windows-x64-local
Exit code: 0. No warning/error lines. Because most targets were already up to date,
a full clean rebuild was then forced so every source is actually recompiled under
warnings-as-error:
    cmake --build --preset windows-x64-local --target clean
    cmake --build --preset windows-x64-local
Exit code: 0. Every translation unit recompiled — output excerpt:
    plugin-main.c / filter.c / coord-map.c / plate-gen.c / shared-state.c /
    geom-resolve.c / watcher.cpp
    streamsentry.vcxproj -> ...RelWithDebInfo\streamsentry.dll
    coord-map-tests.exe / plate-gen-tests.exe / watcher-selftest.exe
Grep of the full clean-rebuild log for warning|error|C4|C5|fatal|LNK: NO matches.

Interpretation: with warnings-as-error ON, a clean full recompile exiting 0 means
ZERO compiler warnings across the whole codebase. CI compile step would pass.
EXACT EXIT CODE: 0.

## Check 2 — Tests: ctest (both pure suites) + watcher-selftest — PASS

Command:
    ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure
Output:
    1/2 Test #1: coord-map-tests ... Passed 0.02 sec
    2/2 Test #2: plate-gen-tests ... Passed 0.14 sec
    100% tests passed, 0 tests failed out of 2
Exit code: 0. Both pure suites PASS.

Command:
    F:\obsplugin\build_x64\RelWithDebInfo\watcher-selftest.exe
Deterministic ok lines observed:
    ok: watcher thread started
    ok: heartbeat advances while watcher alive
    ok: launched notepad.exe
    ok: blocklist rect appeared after notepad opened
    ok: blocklist rect disappeared after notepad closed
    ok: heartbeat frozen while killed (>500ms) -> render fails closed
    ok: heartbeat resumes after un-kill
    INCONCLUSIVE: no toast banner detected (likely DND / focus assist)
    ok: watcher thread stopped cleanly
    watcher-selftest: all deterministic checks passed
Exit code: 0. All deterministic checks (blocklist appear/disappear,
heartbeat freeze>500ms/resume, clean start/stop) passed; toast INCONCLUSIVE
(acceptable — DND/focus-assist). Cleanup: taskkill /F /IM notepad.exe run after;
PowerShell Get-Process notepad confirmed "NO NOTEPAD PROCESSES".

## Check 3 — Shipping DLL hygiene — PASS

Target: build_x64\RelWithDebInfo\streamsentry.dll (64000 bytes, freshly built).
ASCII scan for "PERF watcher tick": count = 0 (ABSENT); no PERF tokens at all.

Command: dumpbin /dependents streamsentry.dll
Dependents:
    obs.dll, dwmapi.dll, ole32.dll, USER32.dll, w32-pthreads.dll, KERNEL32.dll,
    MSVCP140.dll, VCRUNTIME140_1.dll, VCRUNTIME140.dll,
    api-ms-win-crt-{math,heap,string,runtime,stdio}-l1-1-0.dll
Exactly the expected libobs + Windows COM/DWM/pthreads + C++ runtime + CRT stub set.
NO Qt (no Qt6*.dll). NO third-party library. "PERF watcher tick" ABSENT.
(oleaut32/uuid were linked but are import/GUID libs and correctly do not appear as
runtime dependents; required set fully present, nothing extra.)

## Check 4 — Release packaging dry-run — PASS

Install (fresh; prior RelWithDebInfo tree removed first):
    cmake --install build_x64 --prefix F:\obsplugin\release\RelWithDebInfo --config RelWithDebInfo
Exit 0. Installed tree (files):
    streamsentry/bin/64bit/streamsentry.dll
    streamsentry/bin/64bit/streamsentry.pdb
    streamsentry/data/locale/en-US.ini
Layout matches SPEC/task exactly.

Zip (recreated from the fresh install tree): release\streamsentry-0.1.0-windows-x64.zip
Entries (3 file entries + 2 dir entries):
    streamsentry\bin\64bit\streamsentry.dll  (64000 bytes)
    streamsentry\bin\64bit\streamsentry.pdb  (1536000 bytes)
    streamsentry\data\locale\en-US.ini       (353 bytes)
Contains exactly the required tree, nothing extra. DLL size matches the built DLL.

git-ignore: git check-ignore matched release/, build_x64/, .deps/,
CMakeUserPresets.json (all via .gitignore /*). release/ (and the recreated
install+zip) does NOT appear in git status.

## Check 5 — Docs present and consistent — PASS

- README.md present. Covers problem (## The problem), deterministic/no-AI
  ("Deterministic, never AI"), fail-closed ("Fail-closed is the product", cannot
  be turned off), opaque-not-blur ("Opaque, never blur"), Install, Usage,
  Limitations (## Limitations), Roadmap (## Roadmap), license "GPL-2.0-or-later".
- LICENSE: "GNU GENERAL PUBLIC LICENSE / Version 2, June 1991", 338 lines, full
  GPLv2 body incl. Preamble + "How to Apply These Terms". Correct.
- CHANGELOG.md: entries for 0.1.0-m0, 0.1.0-m1, 0.1.0-m2, 0.1.0-m3.
- TESTING.md: sections ## M0, ## M1, ## M2, ## M3 (plus a Post-M0 rename note).
- README install-vs-zip consistency: README download is
  streamsentry-0.1.0-windows-x64.zip and states "The zip mirrors OBS layout:
  streamsentry/bin/64bit/... and streamsentry/data/...". Actual zip is
  streamsentry\bin\64bit\streamsentry.dll + streamsentry\data\locale\en-US.ini —
  README bin/64bit + data layout MATCHES the actual zip.

## Check 6 — buildspec.json fields — PASS

Top-level (buildspec.json lines 39-41):
    "name": "streamsentry"
    "displayName": "StreamSentry"
    "version": "0.1.0"
(Other "version" strings are nested dependency versions: OBS 31.1.1, obs-deps,
Qt6 — not the plugin version.) All three required fields correct.

## Check 7 — git status: only expected M4 changes — PASS

    git status -s
     M README.md
    ?? reports/FINAL.md
- README.md modified (tracked) — 133-insert/45-delete M4 doc rewrite (M0-skeleton
  status -> full ship-ready README); documentation only, no code logic.
- reports/FINAL.md untracked (new M4 capstone report).
- Nothing staged.
- build_x64/, .deps/, release/, CMakeUserPresets.json all ignored and ABSENT from
  status.
(This report, reports/M4-verifier.md, adds one more expected untracked file.)

---

## Not covered by automation (stated explicitly)

Remain MANUAL, NOT exercised by any check above (tracked in
reports/HUMAN_CHECKLIST.md and reports/FINAL.md):
- In-OBS visual confirmation of plate/card look and full opacity (OBS not launched).
- Real toast masking with Do-Not-Disturb OFF (automated toast test is INCONCLUSIVE
  under this machine DND; watcher detects the window, but no banner renders).
- Live password-field masking in a real browser; a real blocklist app
  (1Password/KeePass) if installed.
- Coordinate accuracy on a real second monitor / different DPI and a scaled/cropped
  source (v0.1 fails closed on scaled/window capture by design).
- The full 2-hour endurance soak (only a 30-minute soak was done, in M3).
- Code signing / SmartScreen behavior of the shipped DLL.

## FINAL VERDICT: VERIFIED
