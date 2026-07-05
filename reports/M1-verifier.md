# M1 Verification Report — StreamSentry

- Date: 2026-07-05 (verification run ~16:52-16:54)
- Verifier: automated evidence collection (Claude Code verifier agent)
- HEAD at verification: 8f4244818b802260bb81c82637365010c01b3552 ("Replace template README with actual project description") plus uncommitted M1 working-tree changes (see Check 7)
- Scope: Milestone M1 — mask rendering pipeline driven by fake rects (debug toggles),
  fail-closed path, pure coordinate-mapping module with ctest tests. No real detection
  (that is M2).
- Note: build_x64/ reused the existing configure cache; configure was NOT re-run, per
  instructions. The build step was an incremental no-op (artifact mtimes 16:43-16:48
  predate the ~16:52 build run; SHA identical before/after by construction).

---

## Check 1 — Build + ctest: PASS

Build command (Git Bash, from F:\obsplugin):

    cmake --build --preset windows-x64-local > "<session-scratchpad>/build-m1.log" 2>&1; echo "BUILD EXIT CODE: $?"

Output:

    BUILD EXIT CODE: 0
    .NET Framework MSBuild 17.14.40+3e7442088  [locale-prefixed banner line]
      plugin-support.vcxproj -> F:\obsplugin\build_x64\RelWithDebInfo\plugin-support.lib
      streamsentry.vcxproj -> F:\obsplugin\build_x64\RelWithDebInfo\streamsentry.dll
      Copy streamsentry to rundir    Copy streamsentry resources to rundir
      coord-map-tests.vcxproj -> F:\obsplugin\build_x64\RelWithDebInfo\coord-map-tests.exe
      plate-gen-tests.vcxproj -> F:\obsplugin\build_x64\RelWithDebInfo\plate-gen-tests.exe

Warnings/errors scan: grep -inE 'warning|error' "<session-scratchpad>/build-m1.log" -> no matches (grep exit 1).
No warnings were accepted silently; there were none.

Test command:

    ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure; echo "CTEST EXIT CODE: $?"

Output (verbatim):

    Internal ctest changing into directory: F:/obsplugin/build_x64
    Test project F:/obsplugin/build_x64
        Start 1: coord-map-tests
    1/2 Test #1: coord-map-tests ..................   Passed    0.01 sec
        Start 2: plate-gen-tests
    2/2 Test #2: plate-gen-tests ..................   Passed    0.01 sec

    100% tests passed, 0 tests failed out of 2

    Total Test time (real) =   0.02 sec
    CTEST EXIT CODE: 0

Per-suite: coord-map-tests 1/1 Passed; plate-gen-tests 1/1 Passed. Both required suites
present and passing; exit code 0.

Artifacts (ls -la):

    -rwxr-xr-x 21504 Jul  5 16:44 F:/obsplugin/build_x64/RelWithDebInfo/coord-map-tests.exe
    -rwxr-xr-x 19456 Jul  5 16:43 F:/obsplugin/build_x64/RelWithDebInfo/plate-gen-tests.exe
    -rwxr-xr-x 26624 Jul  5 16:48 F:/obsplugin/build_x64/RelWithDebInfo/streamsentry.dll

VERDICT: VERIFIED

## Check 2 — plate-gen opacity is unit-tested; no blur/pixelate/mosaic: PASS

Opacity assertions (read of F:\obsplugin\tests\plate-gen-tests.c):

- test_toast_card_opacity (lines 42-55): for 5 sizes down to 33x33, asserts
  ss_image_opaque_inside(&img, ss_plate_max_corner_inset()) (line 50) and center pixel
  alpha == 255 (line 52).
- test_privacy_plate_opacity (lines 72-82): same two assertions for 5 plate sizes
  (lines 78-79).
- test_status_banner_fully_opaque (lines 84-93): fail-closed banner checked with
  inset 0 — every single pixel opaque (line 91).
- test_toast_card_corners_transparent (lines 57-70): only the literal rounded-corner
  pixels are transparent; edge midpoints are opaque.
- test_padding_exceeds_corner_inset (line 130): CHECK(ss_plate_max_corner_inset() < 8)
  — the required inset-bound assertion.

Implementation (read of F:\obsplugin\src\plate-gen.c):

- ss_image_opaque_inside (lines 330-343) walks every pixel inside the inset and fails
  on any alpha != 255 — the tests are pixel-exact, not sampled.
- ss_plate_max_corner_inset (lines 345-353) returns 5, with a geometric derivation in
  the comment (radius 12 -> deepest transparent pixel < 4.6px from corner). 5 < 8.
- Filter-side pad SS_MASK_PAD_PX = 12.0 (src/filter.c line 38) satisfies the
  "pad >= 8 > inset" contract.
- All fills write hardcoded RGBA with a=255; the generators synthesize images from
  nothing — no source content is sampled, so no reversible obfuscation is possible.

Forbidden-technique grep (ripgrep, case-insensitive, pattern blur|pixel|mosaic|downscale
over F:\obsplugin\src): ZERO matches for "blur", "mosaic", "downscale". The 14 matches
for "pixel" are all comments about pixel-space coordinates / pixel-level opacity
guarantees (coord-map.c:58; plate-gen.h:24,61,66; shared-state.h:47;
plate-gen.c:75,109,347,348; filter.c:36; coord-map.h:22,24,48,64). No blur/pixelate/
mosaic/downscale operation exists in src/.

VERDICT: VERIFIED

## Check 3 — filter.c fail-closed logic: PASS

Read of F:\obsplugin\src\filter.c (uncommitted M1 file, audited at working-tree state).

### 3a. Threshold is a compile-time constant; no setting bypasses the age check

- Line 34: #define SS_HEARTBEAT_STALE_NS 500000000ULL — 500ms, compile-time constant,
  with a comment stating it is deliberately not exposed anywhere user-configurable.
- The complete settings surface is three booleans: filter_update (lines 86-95) reads
  only "enabled", "debug_rects", "debug_stall"; filter_get_properties (lines 129-143)
  exposes only those three; filter_get_defaults (lines 145-150) defaults the same three.
- filter_video_render health evaluation (lines 367-407) consults NO setting:
  age = now - snap.heartbeat_ns (line 371); unhealthy = !have_snap ||
  age > SS_HEARTBEAT_STALE_NS (line 372). Nothing conditions this on any obs_data value.
- enabled == false (lines 353-358) skips the whole filter — the whole-feature toggle the
  SPEC settings UI requires; it does not run masking with fail-closed weakened.
- debug_stall (filter_video_tick lines 193-206): publishes at most one snapshot, then
  returns every tick WITHOUT touching the heartbeat — the heartbeat ages out and the
  render side fails closed on the age check. It can only TRIGGER fail-closed.
- debug_rects (lines 209-219) selects fake-snapshot vs plain heartbeat publication; both
  branches update the heartbeat via the same shared state the render side reads. Neither
  debug toggle is read anywhere in filter_video_render, so neither can suppress
  fail-closed.

### 3b. Every fail path draws black

draw_fail_closed (lines 302-326) fills the full target w x h with opaque black
(vec4_set(&black, 0, 0, 0, 1) + draw_solid(0, 0, w, h), lines 304-306), then overlays
the status-banner texture. All six fail paths route into it:

| # | Fail path | Set/detected at | Black drawn at |
|---|---|---|---|
| 1 | Heartbeat stale (age > SS_HEARTBEAT_STALE_NS) | line 372, reason "heartbeat stale" line 373 | line 404 |
| 2 | No snapshot ever read (!f->have_snap) | line 372, reason "no detection snapshot" line 373 | line 404 |
| 3 | Geometry invalid (!snap.geometry_valid) | lines 382-385, reason "capture geometry unresolved" | line 404 |
| 4 | Mapping SS_MAP_INVALID | lines 394-397, reason "coordinate mapping failed" | line 404 |
| 5 | obs_source_process_filter_begin returns false | line 409, reason "filter chain bypassed" | line 412 |
| 6 | Plate texture allocation failure (get_plate_texture NULL) | lines 421-422, reason "plate texture allocation failed" | line 425 |

Supporting properties verified in the same read:

- Mapping happens BEFORE the target is rendered (comment + loop, lines 375-401), so a
  mapping failure blacks the frame rather than leaving a partially masked rendered frame.
- Paths 5 and 6 draw black over whatever the target may have rendered.
- SS_MAP_NOT_VISIBLE rects are skipped (line 398) — correct per the coord-map contract
  (rect outside the captured region), distinct from SS_MAP_INVALID which blacks out.

Non-black early-outs (listed for completeness; neither is a detection-health fail path):

- Lines 353-358: !f->enabled -> skip filter (explicit whole-feature user toggle, per SPEC).
- Lines 360-365: target w/h == 0 -> skip filter (target has no video; no frame content
  exists to leak and no dimensions exist to draw onto).

VERDICT: VERIFIED

## Check 4 — coord-map.c: INVALID on failure, padding clamped: PASS

Read of F:\obsplugin\src\coord-map.c + coord-map.h:

- Every unusable input returns SS_MAP_INVALID explicitly — nothing is silently dropped:
  NULL geom/rect/out (lines 31-32); non-finite region or rect (lines 33-34); non-finite
  src dims or pad (lines 35-36); region w/h <= 0 (lines 37-38); src w/h <= 0 (lines
  39-40); negative rect w/h or negative pad (lines 41-42); non-finite arithmetic result
  post-mapping (lines 75-76).
- SS_MAP_NOT_VISIBLE is reserved for genuine no-intersection (lines 55-56, 77-78), so
  callers can distinguish "off-screen" from "broken input".
- Padding is applied on all four sides in source space (lines 64-67) and then clamped to
  source bounds [0, src_w] x [0, src_h] (lines 70-73, comment: "padding may not push mask
  off-source").
- Header contract (coord-map.h lines 57-67) documents SS_MAP_INVALID -> "caller must
  fail closed", matching the filter.c behavior verified in Check 3b row 4.
- The pure module has no OBS/Windows includes (only math.h), which is what makes the
  standalone ctest build possible.
- ctest coverage corroborates (tests/coord-map-tests.c): test_invalid_inputs (lines
  165-185) asserts INVALID for NULL/zero/NaN/negative inputs; test_padding_clamped_at_edges
  (lines 141-152) asserts clamping at 0 and at src bounds; test_padding_never_shrinks
  (lines 154-163) asserts padded contains unpadded; plus multi-monitor offset,
  negative-origin, mixed-DPI, scaled-capture, and partial-overlap cases. All passing
  (Check 1).

VERDICT: VERIFIED

## Check 5 — DLL imports + deployment SHA256: PASS

Command (with MSYS_NO_PATHCONV=1 so Git Bash does not mangle /dependents):

    C:\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe /dependents F:\obsplugin\build_x64\RelWithDebInfo\streamsentry.dll

Full dependency list:

    obs.dll
    w32-pthreads.dll
    VCRUNTIME140.dll
    api-ms-win-crt-math-l1-1-0.dll
    api-ms-win-crt-heap-l1-1-0.dll
    api-ms-win-crt-stdio-l1-1-0.dll
    api-ms-win-crt-runtime-l1-1-0.dll
    KERNEL32.dll

- obs.dll present. w32-pthreads.dll is new vs M0 and expected: src/shared-state.c uses
  the libobs pthread shim (#include <util/threading.h>, pthread_mutex_* at lines 23-70).
- api-ms-win-crt-math-l1-1-0.dll is also new vs M0 (coord-map.c uses math.h
  fmax/fmin/isfinite) — on the acceptable api-ms-win-crt* list.
- Everything else is VCRUNTIME / api-ms-win-crt* / KERNEL32. No Qt6*, no other DLLs.

SHA256 comparison:

    sha256sum F:/obsplugin/build_x64/RelWithDebInfo/streamsentry.dll D:/software/obs/obs-studio/obs-plugins/64bit/streamsentry.dll

    a86b062858c5b353d2b4e9cd95887a12c406e2142edcd4d6b56ef7a200cd4e4b  F:/obsplugin/build_x64/RelWithDebInfo/streamsentry.dll
    a86b062858c5b353d2b4e9cd95887a12c406e2142edcd4d6b56ef7a200cd4e4b  D:/software/obs/obs-studio/obs-plugins/64bit/streamsentry.dll

Build artifact == deployed DLL (both mtime Jul 5 16:48).

VERDICT: VERIFIED

## Check 6 — In-OBS evidence (log file read; OBS not launched): PASS with caveat

File: C:\Users\darker\AppData\Roaming\obs-studio\logs\2026-07-05 16-47-18.txt
(15266 bytes, 249 lines). OBS was NOT launched and nothing under %APPDATA%\obs-studio
was modified during this verification; the log was only read.

All four required lines present, in the required order (grep line numbers + timestamps):

    106: 16:47:19.932: [streamsentry] plugin loaded successfully (version 0.1.0)
    166: 16:47:19.992:         - filter: 'StreamSentry' (streamsentry_filter)
    169: 16:47:20.006: [streamsentry] mask plates active: 2
    172: 16:47:20.523: [streamsentry] FAIL-CLOSED engaged: heartbeat stale (heartbeat age 519 ms)

Timestamp delta "mask plates active" (16:47:20.006) -> "FAIL-CLOSED engaged"
(16:47:20.523) = 517 ms, within the required 450-700 ms window (500 ms threshold plus
frame granularity). The internally measured heartbeat age of 519 ms > 500 ms is
consistent with the log-line delta.

Also observed: line 177 (16:47:24.099: [streamsentry] plugin unloaded) — clean shutdown.

CAVEAT (as instructed): this log was produced by a build one revision older than the
current working tree. The only delta since is a log-wording fix in log_mode that
silences the INIT->NORMAL transition; the fail-closed mechanism (threshold constant,
age check, draw_fail_closed) is unchanged. The log itself corroborates the older-build
provenance: line 168 (16:47:20.006: "[streamsentry] fail-closed cleared: normal
rendering resumed") fires on the very first transition out of INIT, which the current
src/filter.c log_mode (lines 337-341) explicitly silences. Timeline is consistent: log
session 16:47:18-16:47:24; current DLL built/deployed 16:48. Consequence: the exact
current-working-tree binary has NOT itself been observed inside OBS; only the
one-wording-older binary has. All fail-closed code paths at the current tree were
verified by source audit (Check 3); the runtime behavior by this log.

VERDICT: VERIFIED (with the stated provenance caveat)

## Check 7 — Git working tree: PASS

    git -C F:/obsplugin status --short

     M .gitignore
     M CMakeLists.txt
     M data/locale/en-US.ini
     M src/plugin-main.c
    ?? reports/HUMAN_CHECKLIST.md
    ?? src/coord-map.c
    ?? src/coord-map.h
    ?? src/filter.c
    ?? src/filter.h
    ?? src/plate-gen.c
    ?? src/plate-gen.h
    ?? src/shared-state.c
    ?? src/shared-state.h
    ?? tests/

All entries are expected M1 material (verified via git diff):

- .gitignore: adds !/tests to the whitelist (1 insertion).
- CMakeLists.txt: adds the new src files to target_sources + STREAMSENTRY_BUILD_TESTS
  option with enable_testing() and the two add_test targets (coord-map-tests,
  plate-gen-tests), each built from the test file + the pure module only.
- data/locale/en-US.ini: adds DebugGroup/DebugRects/DebugStall strings (labelled
  "M1 scaffolding, removed at M3").
- src/plugin-main.c: M0 inline pass-through filter removed; now registers
  streamsentry_filter_info from filter.c and calls ss_state_init/ss_state_free in
  obs_module_load/obs_module_unload.
- Untracked: the four new src modules + headers, tests/, reports/HUMAN_CHECKLIST.md.
  Nothing unexplained.

Ignore rules still effective:

    git -C F:/obsplugin check-ignore -v CMakeUserPresets.json build_x64 .deps
    .gitignore:2:/*   CMakeUserPresets.json
    .gitignore:2:/*   build_x64
    .gitignore:2:/*   .deps

None of the three appear in git status. (git diff printed informational "LF will be
replaced by CRLF" warnings — line-ending checkout configuration notices, not code or
build warnings.)

VERDICT: VERIFIED

## Not covered by automation

- In-OBS behavior of the exact current-tree binary: the referenced log session was
  produced by the immediately previous revision (wording-only delta in log_mode); no
  in-OBS session of the current binary is on record. Visual appearance of the toast
  card / privacy plate / black+banner inside OBS, and the manual test matrix in SPEC.md,
  require the manual checklist (reports/HUMAN_CHECKLIST.md).
- filter.c and shared-state.c have no unit tests: render/tick logic depends on libobs
  graphics and cannot run under standalone ctest; verified here by source audit (Check 3)
  plus the in-OBS log evidence (Check 6). Only the pure modules (coord-map, plate-gen)
  are ctest-covered.
- Real detection is absent by design at M1: rects come from the debug_rects fake
  provider; window enumeration / UIA / real multi-monitor DPI behavior is M2 and untested
  here (mixed-DPI and multi-monitor cases are covered only as pure-math unit tests).
- Performance (60fps render cost), 2-hour leak run, and the full SPEC acceptance matrix
  remain manual and are not part of M1 verification.

## Verdict

All seven checks passed. Check 6 passed with the provenance caveat stated above.

VERDICT: VERIFIED
