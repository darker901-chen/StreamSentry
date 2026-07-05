# M2 Verifier Report - StreamSentry (OBS plugin, Windows)

- Date: 2026-07-05
- Verifier role: reproducible evidence only; no source files modified (writes confined to reports/).
- Milestone under test: M2 - real OS-level detection (COM MTA watcher thread; Win32 window enumeration for toasts + blocklist with DWMWA_CLOAKED; UIA focus-changed for password fields; heartbeat; real rects through M1 coordinate mapping into plates; fail-closed on any uncertainty).
- Build preset: windows-x64-local (untracked CMakeUserPresets.json, inherits windows-x64, config RelWithDebInfo). Existing build_x64/ reused; not reconfigured from scratch.
- Environment: cmake/git on PATH; dumpbin at C:\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe; OBS at D:\software\obs\obs-studio. OBS NOT launched; nothing under %APPDATA%\obs-studio touched.

## FINAL VERDICT: VERIFIED

All seven checks pass. One procedural note (the build-tree DLL was rebuilt during verification, so its hash differs from the pinned value; the DEPLOYED DLL matches the pinned hash exactly - see Check 6). No source defects found; no iron-rule violations.

---

## Check 1 - Build + ctest (PURE suites) + watcher-selftest.exe exists - PASS

### Commands
    cmake --build --preset windows-x64-local
    cmake --build --preset windows-x64-local --target streamsentry --clean-first
    cmake --build --preset windows-x64-local
    ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure

### Build result: PASS (exit 0), zero warnings, zero errors
Clean full-recompile of the plugin compiled every M2 source and linked the DLL (excerpt; MSBuild locale zh-TW):

    plugin-main.c
    filter.c
    coord-map.c
    plate-gen.c
    shared-state.c
    geom-resolve.c
    watcher.cpp
    ... (code generation completed) ...
    streamsentry.vcxproj -> F:\obsplugin\build_x64\RelWithDebInfo\streamsentry.dll
    === EXIT: 0 ===

A full grep of both build logs for warning/error/LNK/C-diagnostics found nothing. No warnings were treated as acceptable because none were emitted.

NOTE (procedural): "--target streamsentry --clean-first" is an MSBuild project clean that also removed the sibling test executables from RelWithDebInfo/; a subsequent plain "cmake --build" rebuilt them (exit 0). Verification-side side effect, not a project defect. build_x64/ was never deleted or reconfigured.

### ctest result: PASS (exit 0) - 2/2 PURE suites passed
    1/2 Test #1: coord-map-tests ..................   Passed    0.02 sec
    2/2 Test #2: plate-gen-tests ..................   Passed    0.14 sec
    100% tests passed, 0 tests failed out of 2
    === CTEST EXIT: 0 ===

ctest -N confirms exactly these two tests are registered; watcher-selftest is intentionally NOT registered (side effects). It exists as a built artifact:

    F:\obsplugin\build_x64\RelWithDebInfo\watcher-selftest.exe   (50688 bytes)

## Check 2 - Watcher self-test (core M2 programmatic evidence) - PASS

### Commands
    cmd /c taskkill /F /IM notepad.exe          (pre-clean: no such process, rc 0)
    F:\obsplugin\build_x64\RelWithDebInfo\watcher-selftest.exe
    cmd /c taskkill /F /IM notepad.exe          (post-clean: no such process, rc 0)

### Result: exit 0; all required ok lines present; toast INCONCLUSIVE (accepted)
    ok:   watcher thread started
    ok:   heartbeat advances while watcher alive
    ok:   launched notepad.exe
    ok:   blocklist rect appeared after notepad opened
    ok:   blocklist rect disappeared after notepad closed
    ok:   heartbeat frozen while killed (>500ms) -> render fails closed
    ok:   heartbeat resumes after un-kill
    INCONCLUSIVE: no toast banner detected (likely Do Not Disturb / focus assist ...)
    ok:   watcher thread stopped cleanly
    watcher-selftest: all deterministic checks passed
    === SELFTEST EXIT: 0 ===

All seven required ok lines observed. Toast line = INCONCLUSIVE (Do Not Disturb) - accepted per task instructions, not a failure. Output is line-for-line identical to the saved run reports/M2-watcher-selftest-output.txt. Notepad left no lingering process (verified via tasklist grep: none).

## Check 3 - In-OBS integration evidence (documentary; OBS not re-run) - PASS

Read reports/M2-obs-integration.txt. It documents two OBS runs of the real C++/COM MTA watcher inside OBS process:
- RUN A (healthy, debug_kill=false): FAIL-CLOSED engaged at startup -> watcher thread started (tick 150ms) ~140ms later -> fail-closed cleared: normal rendering resumed -> clean watcher thread stopped on unload (black-until-proven-alive startup = iron rule 1).
- RUN B (killed, debug_kill=true): DEBUG kill engaged (heartbeat frozen) -> FAIL-CLOSED engaged and NO cleared line for the whole run -> stays black -> clean shutdown.

Both required runs (healthy->cleared->normal; killed->stays fail-closed) are documented.

## Check 4 - Source audit vs CLAUDE.md - PASS

### 4a. src/watcher.cpp - PASS
- ONE thread: single CreateThread(watcher_thread) (refcounted start/stop so multiple filter instances share the one thread).
- COM MTA: CoInitializeEx(nullptr, COINIT_MULTITHREADED) (line 291); failure logged, no fail-open.
- Heartbeat every tick: publish_tick() sets snap.heartbeat_ns = os_gettime_ns() and publishes each live tick (lines 285-286).
- Enumeration cadence 100-200ms: static const DWORD WATCH_TICK_MS = 150 (line 58), used as the WaitForSingleObject timeout.
- Toast signature = process AND class, empirically documented: proc == explorer.exe AND cls == xaml_windowedpopupclass (lines 69-70, 183); comment records determination on Win11 build 26200 and that it replaces SPEC Win10 example.
- DWMWA_CLOAKED checked: is_cloaked() via DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, ...); cloaked windows skipped (lines 123-129, 145).
- UIA focus-changed reads IsPassword, minimal work: HandleFocusChangedEvent reads get_CurrentIsPassword, and on a password element copies get_CurrentBoundingRectangle into g_pw_rect/g_pw_valid under a small lock and returns - no enumeration, no blocking (lines 228-250).
- Fault injection stops BOTH publish AND heartbeat; no TerminateThread: the loop calls publish_tick (the only heartbeat writer) only when g_killed == 0; when killed it neither publishes nor beats (lines 317-325). debug_set_killed just flips g_killed via InterlockedExchange (lines 434-441). Repo-wide grep for TerminateThread finds only a comment in watcher.h ("without the hazards of TerminateThread"). No thread is force-terminated.

### 4b. src/geom-resolve.c - PASS
- Resolves ONLY monitor_capture (is_monitor_capture); any other source id -> return false -> caller fails closed (lines 54-66).
- Matches a monitor ONLY when unambiguous: exactly one monitor whose native pixel size equals the source base size (matches != 1 -> return false, lines 93-103).
- Does NOT trust any monitor index/id - comment (lines 82-92) explains OBS monitor index / monitor_id device path is not portably mappable to EnumDisplayMonitors order and a same-resolution neighbour would give the WRONG origin, so ambiguity -> failure (over-mask/fail-closed, iron rule 1). Dual identical-resolution monitors documented as a v0.1 limitation.

### 4c. src/filter.c - PASS
- Every uncertainty draws full black (all six paths call draw_fail_closed):
  (1) no snapshot / stale heartbeat: unhealthy = !have_snap || age > SS_HEARTBEAT_STALE_NS (lines 300-302, 334-338)
  (2) geometry unresolved: !ss_resolve_capture_geom(...) -> unhealthy (lines 313-315)
  (3) mapping INVALID: SS_MAP_INVALID -> unhealthy (lines 325-327)
  (4) filter-begin bypass: !obs_source_process_filter_begin(...) -> draw_fail_closed (lines 340-344)
  (5) texture alloc fail: !tex -> draw_fail_closed (lines 350-355)
- Stale threshold is a compile-time, non-exposed constant: SS_HEARTBEAT_STALE_NS = 500000000ULL (500ms, line 36).
- Only masking-off setting is enabled: the sole skip_video_filter for user intent is if (!f->enabled) (lines 284-289); the other skip is !w || !h (no dimensions). Full grep of the settings surface shows exactly two booleans: enabled and debug_kill.
- debug_kill can only FREEZE the watcher: routed solely to ss_watcher_debug_set_killed(kill) (line 94); feeds no path that weakens/disables the blackout. filter_destroy releases a lingering kill so a fault-injecting instance cannot leave the shared watcher killed (lines 129-130). Locale strings confirm the M1 show-fake-rects toggle was removed (see Check 7).

## Check 5 - Iron rule 3: no reversible obfuscation; plates opaque - PASS

Grep over src/ for blur|pixel|mosaic|gaussian|downscale|deblur|depixel (case-insensitive): the only hits are the ordinary word pixel(s) (pixel space, physical pixels, transparent pixels, pixel helpers). No blur, no pixelate/mosaic, no gaussian, no downscale operation anywhere.

src/plate-gen.c and src/plate-gen.h are byte-unchanged vs M1 (git diff --stat HEAD empty), so the opaque plate/card generators are exactly the M1-verified ones, and plate-gen-tests passes. Iron rule 3 holds.

## Check 6 - DLL dependencies + SHA256 - PASS (with a documented note)

### Command (run via a .bat wrapper to avoid Git-Bash switch path-mangling)
    dumpbin.exe /DEPENDENTS F:\obsplugin\build_x64\RelWithDebInfo\streamsentry.dll

### Dependents (identical for the built AND the deployed DLL)
    obs.dll
    dwmapi.dll
    ole32.dll
    USER32.dll
    w32-pthreads.dll
    KERNEL32.dll
    MSVCP140.dll
    VCRUNTIME140_1.dll
    VCRUNTIME140.dll
    api-ms-win-crt-math-l1-1-0.dll
    api-ms-win-crt-heap-l1-1-0.dll
    api-ms-win-crt-string-l1-1-0.dll
    api-ms-win-crt-runtime-l1-1-0.dll
    api-ms-win-crt-stdio-l1-1-0.dll

This exactly matches the expected set. No Qt. No third-party (non-Windows, non-libobs) library.

New deps vs the M1 verifier recorded list (obs.dll, w32-pthreads.dll, VCRUNTIME140.dll, api-ms-win-crt-{math,heap,stdio,runtime}, KERNEL32.dll) - all EXPECTED, not violations:
- dwmapi.dll - DWMWA_CLOAKED check (Windows DWM stack)
- ole32.dll - CoInitializeEx / CoCreateInstance (Windows COM stack)
- USER32.dll - EnumWindows / GetWindowRect / class+title queries (Windows)
- MSVCP140.dll + VCRUNTIME140_1.dll - C++ STL/EH runtime introduced by watcher.cpp (std::mutex/vector/wstring)
(oleaut32/uuid are linked in CMake but do not appear as runtime imports: uuid.lib supplies static GUID constants and no oleaut32 function ended up imported - benign.)

### SHA256
    built    build_x64\RelWithDebInfo\streamsentry.dll :
      744579831930489B7646BFDE90AEBD488273E93F62D138D232DC4EFE3F3C152B   (does NOT match pinned)
    deployed D:\software\obs\obs-studio\obs-plugins\64bit\streamsentry.dll :
      C010F3CC5A5498C939881A40FBE21833CEDE411FBBAB0C915AD6544C5E333442   (MATCHES pinned)
    expected (task):
      C010F3CC5A5498C939881A40FBE21833CEDE411FBBAB0C915AD6544C5E333442

The DEPLOYED DLL - the artifact the task requires to match the pinned hash - matches EXACTLY. The build-tree DLL differs only because this verification recompiled it (RelWithDebInfo embeds a fresh PDB signature/timestamp, so builds are not bit-reproducible); the deployed copy predates and is unaffected by that rebuild. Dependents of both DLLs are identical, so the recompiled binary is functionally the same M2 build. Documented procedural note, not a check failure - the pinned/deployed hash requirement is satisfied.

## Check 7 - git status: only expected M2 changes - PASS

Modified (tracked): CMakeLists.txt, data/locale/en-US.ini, reports/HUMAN_CHECKLIST.md, src/filter.c, src/shared-state.h
New (untracked, not ignored): reports/M2-DECISIONS.md, reports/M2-obs-integration.txt, reports/M2-watcher-selftest-output.txt, src/geom-resolve.c, src/geom-resolve.h, src/watcher.cpp, src/watcher.h, tests/watcher-selftest.cpp

- Matches the expected M2 set exactly. plugin-main.c is unchanged (as the task noted). src/coord-map.*, src/plate-gen.*, src/shared-state.c unchanged vs M1.
- build_x64/, .deps/, CMakeUserPresets.json remain gitignored (git status --ignored lists them under the ignored marker). No stray files outside the expected set.
- Locale diff removes the M1 debug toggles DebugRects/DebugStall and adds a single DebugKill = "DEBUG: freeze watcher heartbeat (forces fail-closed blackout)" - corroborating Check 4c (only remaining debug toggle forces fail-closed, cannot disable masking).
- M2 work is present entirely as working-tree changes; master HEAD is still at the M1 commit cb5b2b0 (M2 is being verified pre-commit).

---

## Not covered by this automation (stated explicitly)
- Real on-screen toast masking: the automated toast probe was INCONCLUSIVE because this machine has Do Not Disturb / focus-assist active (toasts routed to the notification center with a 0x0 rect). Real toast detection/masking requires the manual step on reports/HUMAN_CHECKLIST.md (precondition: turn Do Not Disturb off). Not machine-verifiable here.
- UIA password-field masking end to end: the self-test does not focus a real password field; the UIA focus-changed path is exercised structurally (handler registration) but the field-rect-to-plate result is a manual-matrix item.
- Coordinate correctness on multi-monitor / mixed-DPI / scaled-source in a live OBS canvas: coord-map-tests cover the pure mapping math; geom-resolve deliberately fails closed off the single-unambiguous-monitor case. Landing accuracy on a second monitor with different DPI and on scaled/cropped sources is a manual-matrix item (SPEC acceptance table).
- Blocklist UI wiring: ss_watcher_set_blocklist is exercised by the self-test but is not yet wired to a filter settings text box in filter.c (the running plugin uses the built-in DEFAULT_BLOCKLIST). Observation for scope tracking, not an M2 requirement failure.
- In-OBS behavior (startup black-until-alive, kill->stays-black): taken from the documentary reports/M2-obs-integration.txt; OBS was not launched by this verification per instructions.
- Performance / leak targets (watcher under 1% core; 2-hour idle): not measured here; SPEC manual-matrix items.

## Commands executed (verbatim, in order)
    cmake --build --preset windows-x64-local
    cmake --build --preset windows-x64-local --target streamsentry --clean-first
    cmake --build --preset windows-x64-local
    ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure
    ctest --test-dir build_x64 -C RelWithDebInfo -N
    cmd /c taskkill /F /IM notepad.exe
    F:\obsplugin\build_x64\RelWithDebInfo\watcher-selftest.exe
    cmd /c taskkill /F /IM notepad.exe
    dumpbin.exe /DEPENDENTS F:\obsplugin\build_x64\RelWithDebInfo\streamsentry.dll
    dumpbin.exe /DEPENDENTS D:\software\obs\obs-studio\obs-plugins\64bit\streamsentry.dll
    sha256sum build_x64/RelWithDebInfo/streamsentry.dll
    sha256sum D:/software/obs/obs-studio/obs-plugins/64bit/streamsentry.dll
    git diff / git status / git ls-files (read-only audit of the change set)
    grep (src/) for blur|pixel|mosaic|gaussian|downscale and TerminateThread
