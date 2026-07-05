# M1 Spec-Guardian Audit — StreamSentry

- Date: 2026-07-05
- Auditor: Spec Guardian (Claude Code)
- Authority: CLAUDE.md iron rules + SPEC.md v0.1 scope (single source of truth; not restated here)
- Change set audited: uncommitted working-tree M1 changes vs HEAD (M0/rename state).
  New: src/coord-map.{c,h}, src/plate-gen.{c,h}, src/shared-state.{c,h}, src/filter.{c,h},
  tests/coord-map-tests.c, tests/plate-gen-tests.c, reports/{HUMAN_CHECKLIST,M1-verifier}.md.
  Modified: src/plugin-main.c, CMakeLists.txt, data/locale/en-US.ini, .gitignore.
- M1 contract: mask-render pipeline driven by HARDCODED FAKE rects (real detection is M2),
  fail-closed path end-to-end, coordinate mapping as a pure unit-testable module.
- Build/test/opacity/fail-closed runtime evidence cited from reports/M1-verifier.md
  (VERDICT: VERIFIED); not re-run here.

---

## Rule 1 — Fail-closed is the product; NOT user-disableable

PASS.

Settings surface is exactly three booleans and NOTHING conditions health on them:
- filter_update reads only "enabled", "debug_rects", "debug_stall" (src/filter.c:86-95).
- filter_get_properties exposes only those three (src/filter.c:129-143).
- filter_get_defaults defaults only those three (src/filter.c:145-150).

Only "enabled" stops masking, and it stops the WHOLE filter (obs_source_skip_video_filter,
src/filter.c:353-358) — the SPEC-sanctioned enable/disable checkbox, not a weakening of
fail-closed. There is no "disable fail-closed" control (SPEC Settings UI: "No option to
disable fail-closed behavior").

500ms threshold is a compile-time constant with no settings path:
- SS_HEARTBEAT_STALE_NS 500000000ULL (src/filter.c:34); comment states it is deliberately
  not exposed anywhere user-configurable.
- Health test reads no obs_data: age = now - snap.heartbeat_ns (src/filter.c:371);
  unhealthy = !have_snap OR age > SS_HEARTBEAT_STALE_NS (src/filter.c:372).

Every uncertainty path lands in full black via draw_fail_closed (src/filter.c:302-326: vec4
black a=1.0 + draw_solid over full w x h, then status banner overlay):
1. No snapshot ever read (!have_snap) -> src/filter.c:372, black at :404.
2. Stale heartbeat (age > threshold) -> src/filter.c:372, black at :404.
3. Geometry invalid (!geometry_valid) -> src/filter.c:382-385, black at :404.
4. Mapping SS_MAP_INVALID -> src/filter.c:394-397, black at :404.
5. obs_source_process_filter_begin false -> src/filter.c:409-415, black at :412.
6. Plate texture alloc fail (get_plate_texture NULL) -> src/filter.c:421-428, black at :425.
Mapping runs BEFORE the target is rendered (src/filter.c:375-401), so a mapping failure blacks
the whole frame instead of partially masking. SS_MAP_NOT_VISIBLE (rect outside capture) is
correctly skipped, not blacked (src/filter.c:398) — that is "no rect here", not a health fail.

Two non-black early-outs, both correct and neither a health path: !enabled (whole-feature user
toggle, src/filter.c:353-358) and target w/h == 0 (no video exists to leak, no surface to draw
onto, src/filter.c:360-365).

DEBUG scaffolding cannot disable or weaken fail-closed (developer-only, marked for M3 removal):
- debug_stall: emits at most one snapshot then returns every tick WITHOUT touching the
  heartbeat, so it ages out and render fails closed on age (src/filter.c:193-206). It can ONLY
  trigger fail-closed.
- debug_rects: selects fake-snapshot vs plain-heartbeat publication; both update the same
  shared heartbeat the render side reads (src/filter.c:209-219). It only injects fake rects.
- NEITHER toggle is read anywhere in filter_video_render — the render/health path is
  independent of them, so neither can suppress the black-out.
- Marked developer-only / M3-removal in code (src/filter.c:61-63, :136-137), the properties
  group label, and locale (data/locale/en-US.ini "Developer (M1 scaffolding, removed at M3)").
  These are dev fault-injection controls, not a user-facing off switch for fail-closed.

## Rule 2 — Deterministic only; no ML/AI

PASS. Grep over src/ for tensor|onnx|inference|neural|ml|opencv|torch|tflite|model returned
zero matches. All rects come from hand-written fake geometry (src/filter.c:152-181); all plate
visuals are procedurally drawn from constants (src/plate-gen.c). No model, classifier, or
probabilistic logic of any kind.

## Rule 3 — Opaque masking; never blur/pixelate/mosaic; two styles; raw black reserved

PASS.

Opacity is enforced, not eyeballed: ss_image_opaque_inside walks EVERY pixel inside the inset
and fails on any alpha != 255 (src/plate-gen.c:330-343). Tests assert this pixel-exact for
toast cards and privacy plates down to 32-33px, plus center pixel == 255
(tests/plate-gen-tests.c:42-82); the fail-closed banner is asserted fully opaque at inset 0
(tests/plate-gen-tests.c:84-93). Verifier ran these: 2/2 ctest suites passed
(reports/M1-verifier.md Checks 1 and 2).

The only transparent pixels are the slivers outside the rounded corners, and that gap is
bounded: ss_plate_max_corner_inset() == 5 (src/plate-gen.c:345-353), the filter pads every rect
by SS_MASK_PAD_PX = 12.0 (src/filter.c:38), and a test pins pad-bound > inset
(ss_plate_max_corner_inset() < 8, tests/plate-gen-tests.c:125-131). The sensitive rect thus sits
strictly inside the opaque region — corners never expose content.

Kind to style mapping is correct (src/filter.c:265-268 get_plate_texture): SS_RECT_TOAST ->
ss_gen_toast_card (notification card: rounded rect + bell + "Notification hidden");
SS_RECT_WINDOW and SS_RECT_FIELD both -> ss_gen_privacy_plate (dark fill + lock + "Hidden").
Matches SPEC Masking: toast rects get the card, window/field rects get the privacy plate.

Raw black is used ONLY in draw_fail_closed (src/filter.c:302-306, the sole vec4(0,0,0,1) solid
fill). Normal masks are always the generated opaque textures (src/filter.c:418-430), never
black. Grep over src/ for blur|gaussian|pixelate|mosaic|downscale|downsample returned zero
operational matches (the one opacity hit is a comment naming the security property,
plate-gen.h:20). No reversible obfuscation exists — generators synthesize from constants and
never sample source content, so archived-clip deblur/depixel attacks are structurally impossible.

## Rule 4 — No third-party dependencies beyond the template

PASS.

CMake adds only new first-party src/ files to the existing target plus a test option; it adds
no find_package for any external library (CMakeLists.txt:37-62). ENABLE_QT stays OFF
(CMakeLists.txt:8). The only link deps beyond M0 (verifier dumpbin, reports/M1-verifier.md
Check 5) are w32-pthreads.dll and api-ms-win-crt-math. w32-pthreads is the libobs pthread shim,
pulled transitively via util/threading.h (src/shared-state.c:23); the CRT math DLL comes from
coord-map.c math.h (fmax/fmin/isfinite). No Qt6*, no other DLLs. coord-map and plate-gen are
dependency-free: coord-map.c includes only math.h; plate-gen.c only stdlib.h and string.h. That
dependency-freedom is what lets the two ctest binaries build without libobs. Shipped-plugin
dependency posture is unchanged in kind (libobs + Windows/CRT only).

## Rule 5 — Scope locked to v0.1; no backlog, no premature features

PASS.

No real detection has landed (correct for M1; detection is M2): grep over src/ for
EnumWindows|EnumChildWindows|UIAutomation|IUIAutomation|SetWinEventHook|CreateThread|
_beginthread|CoInitialize|CoCreateInstance|DwmGetWindowAttribute|DWMWA_CLOAKED|GetWindowRect|
pthread_create|windows.h all returned zero matches. Rects are hardcoded fakes
(src/filter.c:152-181) published from video_tick standing in for the M2 watcher.

No backlog/out-of-scope items: grep over src/ for allowlist|whitelist|tray|auto-update|macos|
per-app|focus-assist|blur returned zero matches. No source style beyond toast card / privacy
plate. The fake-rect provider and both DEBUG toggles are clearly M1 scaffolding slated for M3
removal (src/filter.c:61-63, :136-137, :188-192; locale label), i.e. developer tooling, not
shipping features.

## Rule 6 — GPLv2-or-later headers; original code

PASS. All 9 M1 src/ files and both tests/ files carry the full GPL header with the exact
"either version 2 of the License, or (at your option) any later version" phrasing (header block
lines 1-17 of each file; verified via grep across all 11 files). Matches the 2026-07-05
GPL-2.0-or-later decision in CLAUDE.md rule 6. Repo LICENSE present. Code reads as original:
hand-rolled 5x7 bitmap font drawn for this project (src/plate-gen.c:24-64), first-principles
rect intersect/scale/clamp math (src/coord-map.c), straightforward libobs filter boilerplate
against documented OBS APIs. No third-party source signatures, attribution blocks, or copied
idioms observed.

(Auditor note: an initial automated header check using a 3-line window falsely reported the
headers missing; the phrase sits on line 5. Re-checked with a correct window across all 11
files — every file has it. Recording the false positive for transparency; it is NOT a finding.)

## Architecture conformance (CLAUDE.md Architecture fixed)

PASS.

- Shared state = rect list + heartbeat timestamp (struct ss_snapshot: heartbeat_ns,
  geometry_valid + geom, num_rects + fixed rects[SS_MAX_RECTS]; src/shared-state.h:52-58).
- Render-side critical section is NON-BLOCKING: ss_state_try_read uses pthread_mutex_trylock and
  returns false on contention instead of blocking (src/shared-state.c:63-72). On a missed read
  the caller keeps its previous snapshot and staleness is still governed by the heartbeat age
  check — contention degrades toward fail-closed, never toward stale-open.
- Critical section is TINY: the reader holds the lock only for one fixed-size struct copy
  (src/shared-state.c:69); writers (publish / touch_heartbeat) hold it only for a copy or a
  single field write (src/shared-state.c:45-61). Plate generation, texture caching, and drawing
  all happen outside any lock.
- Writer-side interface (ss_state_publish full snapshot; ss_state_touch_heartbeat heartbeat
  only) matches the fixed contract: event handlers copy rects, post to shared state, return.
- The watcher THREAD itself is deferred to M2 (M1 uses video_tick as the fake provider) — this
  is expected per the M1 contract, not a violation. Nothing in the M1 design blocks the M2
  watcher from slotting in: the writer API is thread-agnostic (plain mutex-guarded publish),
  screen coords are physical virtual-screen pixels ready for a real enumerator
  (src/shared-state.h:47), and ss_capture_geom already carries the per-capture region/source
  ratio the coordinate chain needs. M2 swaps the video_tick provider for the COM MTA thread
  calling the same ss_state_publish; no shared-state redesign is implied.

## Build/CI posture note (CMakeLists.txt + .gitignore)

PASS. .gitignore adds !/tests (un-ignores the tests dir for VCS only; no build/CI effect).
CMakeLists.txt adds the new first-party src to the existing MODULE target and a
STREAMSENTRY_BUILD_TESTS option (default ON) building two standalone test exes from the test
file + the pure module .c, wired via enable_testing()/add_test. Tests link no libobs and ship
nothing into the plugin. Shipped-DLL dependency posture unchanged in kind; verifier confirmed
build == deployed DLL by SHA256 (reports/M1-verifier.md Check 5).

## Non-blocking QUESTIONS for the human

None that block the milestone. For awareness only (consistent with the M1 contract and already
disclosed by the verifier): the exact current-tree binary was not itself observed inside OBS;
the referenced OBS log came from a one-revision-older build whose only delta is log wording in
log_mode (reports/M1-verifier.md Check 6 caveat). reports/HUMAN_CHECKLIST.md M1 covers the
on-screen visual confirmation. Not an iron-rule matter.

---

RESULT: PASS
