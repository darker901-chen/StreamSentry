# M3 Verifier Report — StreamSentry (OBS plugin, Windows)

- Date: 2026-07-05
- Verifier role: reproducible evidence only; no source files modified (writes confined to reports/).
- Milestone under test: M3 — settings UI (Enable checkbox + multi-line Blocklist textbox, no fail-closed-disable option), removal of the developer debug toggle, and hardening (perf measurement, 30-min soak, acceptance-matrix mapping).
- Build preset: windows-x64-local (untracked CMakeUserPresets.json, inherits windows-x64, config RelWithDebInfo). Existing build_x64/ reused; not reconfigured. Currently-configured build has STREAMSENTRY_PERF_LOG=OFF (shipping/release config).
- Environment: cmake/git on PATH; dumpbin at C:\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe; OBS at D:\software\obs\obs-studio. OBS NOT launched; nothing under %APPDATA%\obs-studio touched. Soak NOT re-run.

## FINAL VERDICT: VERIFIED

All eight checks pass. No source defects found; no iron-rule violations. One benign, load-bearing observation: because filter.c no longer calls ss_watcher_debug_set_killed, the linker (/OPT:REF) dropped that unreferenced function and its log string from the shipping DLL — the debug path is genuinely absent from the release artifact while the function still exists in source and in the self-test binary (Check 4).

---

## Check 1 — Build + ctest (pure suites) + watcher-selftest — PASS

Commands:
    cmake --build --preset windows-x64-local
    ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure
    cmd /c taskkill /F /IM notepad.exe (pre-clean)
    F:\obsplugin\build_x64\RelWithDebInfo\watcher-selftest.exe
    cmd /c taskkill /F /IM notepad.exe (post-clean)

Build result: PASS (exit 0). Linked streamsentry.dll + coord-map-tests.exe + plate-gen-tests.exe + watcher-selftest.exe. No warnings in the incremental build tail; nothing treated as acceptable.

ctest result: PASS (exit 0), 2/2 pure suites:
    1/2 Test #1: coord-map-tests .......... Passed 0.01 sec
    2/2 Test #2: plate-gen-tests .......... Passed 0.01 sec
    100% tests passed, 0 tests failed out of 2

watcher-selftest: exit 0. All deterministic ok lines present (thread started; heartbeat advances; notepad launched; blocklist rect appeared after open; blocklist rect disappeared after close; heartbeat frozen while killed >500ms -> render fails closed; heartbeat resumes after un-kill; thread stopped cleanly; "all deterministic checks passed"). Toast line INCONCLUSIVE (Do Not Disturb) — accepted. Notepad pre/post cleaned; tasklist reports no matching notepad task.

## Check 2 — Release build has perf compiled OUT + deployed==built hash — PASS

- ASCII "PERF watcher tick": NOT FOUND in build_x64/RelWithDebInfo/streamsentry.dll (also "PERF render decision" NOT FOUND). Perf instrumentation compiled out.
- CMakeCache.txt: STREAMSENTRY_PERF_LOG:BOOL=OFF (the currently-configured build is the shipping config).
- SHA256 built build_x64 DLL:  9835ca172898c46672e7eb9e08e7953be107696797a93a26584b5c998ef0cf12
- SHA256 deployed OBS DLL:      9835ca172898c46672e7eb9e08e7953be107696797a93a26584b5c998ef0cf12
- Expected pinned (task):       9835CA172898C46672E7EB9E08E7953BE107696797A93A26584B5C998EF0CF12
Deployed and freshly-built copies MATCH each other and the pinned value. The incremental build did not relink the DLL (unchanged inputs), so the RelWithDebInfo PDB signature and hash held — no divergence this session.

## Check 3 — Settings UI source audit (src/filter.c) — PASS

Verified against source and git diff src/filter.c (M2 -> M3):
- 3a. filter_get_properties adds EXACTLY two properties: bool "enabled" (obs_properties_add_bool) and OBS_TEXT_MULTILINE "blocklist" (obs_properties_add_text, with a long-description hint). The M2 developer group and its debug_kill bool were REMOVED. No debug/kill/developer property; nothing that could disable fail-closed.
- 3b. filter_get_defaults sets enabled=true (obs_data_set_default_bool) and blocklist=ss_watcher_default_blocklist_text() (obs_data_set_default_string). The debug_kill default was removed.
- 3c. filter_update reads blocklist via obs_data_get_string and calls ss_watcher_set_blocklist(); the M2 debug_kill / ss_watcher_debug_set_killed block was removed. grep of filter.c for debug|kill|developer: NO matches.
- 3d. struct ss_filter no longer has a debug_kill field (diff deletes that member). The only conditional members are perf fields under #ifdef STREAMSENTRY_PERF_LOG.
- Locale corroboration (data/locale/en-US.ini diff): DebugGroup / DebugKill strings removed; Blocklist + BlocklistHint added.
- Artifact corroboration: the setting-name string debug_kill is ABSENT from streamsentry.dll; blocklist is PRESENT.

## Check 4 — Fail-closed integrity intact (src/filter.c) — PASS

- 500ms threshold is a compile-time constant, not user-exposed: SS_HEARTBEAT_STALE_NS = 500000000ULL via #define (filter.c line 36).
- Six uncertainty paths all reach full black via draw_fail_closed:
  (1) no snapshot (have_snap false) and (2) stale heartbeat (age over SS_HEARTBEAT_STALE_NS) -> unhealthy -> draw_fail_closed (line 351).
  (3) capture geometry unresolved (ss_resolve_capture_geom false) -> unhealthy (line 314) -> draw_fail_closed (351).
  (4) coordinate mapping SS_MAP_INVALID -> unhealthy (line 326) -> draw_fail_closed (351).
  (5) filter-begin bypass (obs_source_process_filter_begin false) -> draw_fail_closed (line 357).
  (6) plate texture allocation failure (tex NULL) -> draw_fail_closed (line 368).
- enabled is the only masking-off switch: the sole user-intent skip is the (f->enabled false) -> obs_source_skip_video_filter branch (line 284); the only other skip is the no-dimensions guard. No user toggle weakens or disables the blackout.
- ss_watcher_debug_set_killed still EXISTS in watcher.cpp (definition line 473) and is declared in watcher.h (line 53); the g_killed fault-injection flag is confined to watcher.cpp (lines 91,322,381,475). It is NOT referenced in filter.c or plugin-main.c; its only caller is tests/watcher-selftest.cpp (lines 173,178). Not wired to any OBS setting.
- Shipping-DLL evidence: DEBUG-kill log strings are ABSENT from streamsentry.dll (the unreferenced function and its narrow string were dropped by /OPT:REF because no plugin TU calls it), while watcher-selftest.exe — which links watcher.cpp separately and DOES call it — contains the DEBUG kill string (count 1). Confirms the function exists yet is unwired in the release artifact.

## Check 5 — Perf / soak / acceptance-matrix evidence — PASS

reports/M3-perf.md reports:
- Watcher tick ~0.55% of one core (samples 0.803-0.826 ms/tick at 150ms cadence) — under the SPEC < 1%-of-one-core target.
- Render decision cost avg 0.03 us/frame (~30 ns), ~0.0002% of a 60fps 16667us budget — no measurable frame-drop risk.
- 30-min soak conclusion: stable, no leak. Working set 345 MB at startup, trimmed to ~320-321 MB by ~160s and flat for the remaining 27 min (max 321.1); private bytes ~356.5 MB flat; handles oscillate 6351-6382 (no monotonic growth); threads 234-242 stable (single watcher thread created once).

Cross-check vs reports/M3-soak-samples.csv (60 samples, 40s..1811s, then SOAK DONE 07/05/2026 18:31:42):
- working_set_mb min/max across samples = 320.0 / 345.3; the 345.3 is only the 40s startup sample; from 160s it sits 320.0-321.1 with no upward trend. Flat, matches the report.
- private_mb min/max = 356.0 / 356.8 — essentially flat ~356.5, no climb.
- handles and threads confined to narrow bands (6351-6382 / 234-242), no monotonic growth.
Raw data corroborates the flat ~320-321 MB, no-leak conclusion.

reports/M3-acceptance-matrix.md EXISTS and covers all 10 SPEC acceptance rows (numbered 1..10, matching SPEC.md): 2 fully machine-verified (row 7 fault-injection, row 10 pass-through), the rest mapped to UNIT / MECHANISM / HUMAN evidence, with the longer SPEC targets (2-hour soak) routed to HUMAN_CHECKLIST.

## Check 6 — Iron rule 3: no reversible obfuscation; plates opaque — PASS

src grep (case-insensitive) for blur|pixelate|mosaic|gaussian|deblur|depixel|downscale: NO matches. plate-gen-tests passes (Check 1). No blur/pixelate/mosaic anywhere in src/.

## Check 7 — DLL dependencies unchanged from M2 — PASS

dumpbin /DEPENDENTS (via a .bat wrapper, to avoid Git-Bash mangling the /DEPENDENTS switch into a path) on both the built and deployed DLL returns an IDENTICAL set:
    obs.dll, dwmapi.dll, ole32.dll, USER32.dll, w32-pthreads.dll, KERNEL32.dll,
    MSVCP140.dll, VCRUNTIME140_1.dll, VCRUNTIME140.dll,
    api-ms-win-crt-math-l1-1-0.dll, api-ms-win-crt-heap-l1-1-0.dll,
    api-ms-win-crt-string-l1-1-0.dll, api-ms-win-crt-runtime-l1-1-0.dll,
    api-ms-win-crt-stdio-l1-1-0.dll
Byte-for-byte the same import set the M2 verifier recorded. No Qt. No third-party (non-Windows, non-libobs) library. Unchanged from M2.

## Check 8 — git status: only expected M3 changes — PASS

    Modified (tracked): CMakeLists.txt, data/locale/en-US.ini, reports/HUMAN_CHECKLIST.md, src/filter.c, src/watcher.cpp, src/watcher.h
    Untracked (not ignored): reports/M3-acceptance-matrix.md, reports/M3-perf.md, reports/M3-soak-samples.csv
    Ignored: build_x64/, .deps/, CMakeUserPresets.json
Matches the expected M3 set exactly. Diffs reviewed:
- CMakeLists.txt: adds option(STREAMSENTRY_PERF_LOG ... OFF) gating the perf compile-definition (default OFF).
- watcher.h: adds the ss_watcher_default_blocklist_text() declaration; retains the fault-injection hook prototype/comment.
- watcher.cpp: adds ss_watcher_default_blocklist_text() and the set_blocklist parsing; ss_watcher_debug_set_killed retained.
- data/locale/en-US.ini: Debug strings out, Blocklist / BlocklistHint in.
This report (reports/M3-verifier.md) is the only additional new file — the verifier own output, permitted.

---

## Not covered by this automation (stated explicitly)
- Real on-screen toast masking end-to-end: the automated toast probe was INCONCLUSIVE (machine Do Not Disturb / focus assist suppresses the banner, per M2-DECISIONS.md). Real toast detection/masking is a manual step on reports/HUMAN_CHECKLIST.md (precondition: DND off). Acceptance-matrix row 1.
- UIA password-field masking end-to-end (row 2): the self-test does not focus a live password field; the focus-changed path is exercised structurally only.
- Credential dialog / UAC (row 3): UAC runs on the secure desktop (not capturable by OBS) — inherently manual.
- Multi-monitor / mixed-DPI / scaled-source landing accuracy in a live canvas (rows 5,6): coord-map-tests cover the pure math; geom-resolve fails closed off the single-unambiguous-monitor case. Real-device placement is a manual-matrix item.
- Visual no-dropped-frames during recording (row 8) and the full 2-hour soak (row 9): only the CPU decision cost and a 30-minute soak were measured this session; the visual and 2-hour confirmations are on HUMAN_CHECKLIST.
- In-OBS runtime behavior (startup black-until-alive; the settings UI as rendered in the OBS properties dialog): OBS was not launched per instructions.
- Perf/soak numbers are read from reports/M3-perf.md + reports/M3-soak-samples.csv (produced from a separate PERF_LOG=ON build); this session did not re-run the soak.

## Commands executed (verbatim, in order)
    cmake --build --preset windows-x64-local
    ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure
    grep -a build_x64/RelWithDebInfo/streamsentry.dll  (PERF watcher tick ; PERF render decision)
    sha256sum build_x64/RelWithDebInfo/streamsentry.dll
    sha256sum D:/software/obs/obs-studio/obs-plugins/64bit/streamsentry.dll
    dumpbin.exe /DEPENDENTS build_x64\RelWithDebInfo\streamsentry.dll        (via .bat wrapper)
    dumpbin.exe /DEPENDENTS D:\...\obs-plugins\64bit\streamsentry.dll        (via .bat wrapper)
    grep (src/) blur|pixelate|mosaic|gaussian|deblur|depixel|downscale
    grep (src/filter.c) debug|kill|developer   ;   grep (src/) ss_watcher_debug_set_killed|g_killed
    grep (tests/) ss_watcher_debug_set_killed
    grep -i STREAMSENTRY_PERF_LOG build_x64/CMakeCache.txt
    cmd /c taskkill /F /IM notepad.exe   (pre-clean)
    F:\obsplugin\build_x64\RelWithDebInfo\watcher-selftest.exe
    cmd /c taskkill /F /IM notepad.exe   (post-clean)   ;   tasklist notepad filter
    git status ; git status --ignored ; git diff (read-only audit of CMakeLists.txt, filter.c, watcher.cpp, watcher.h, en-US.ini)
    read reports/M3-perf.md ; reports/M3-soak-samples.csv ; reports/M3-acceptance-matrix.md ; SPEC.md
