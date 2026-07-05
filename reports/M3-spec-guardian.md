# M3 Spec-Guardian Report - StreamSentry

- Date: 2026-07-05
- Auditor role: Spec Guardian. Loyalty is to CLAUDE.md + SPEC.md only. No source modified.
- Milestone: M3 - Settings UI (Enable + multi-line Blocklist textbox, no fail-closed disable),
  removal of the developer debug scaffolding, and hardening (perf, soak, acceptance-matrix mapping).
- Change set audited: uncommitted working-tree diff since HEAD = 127781c (M2 real detection).
  Files: CMakeLists.txt, data/locale/en-US.ini, src/filter.c, src/watcher.cpp, src/watcher.h,
  reports/HUMAN_CHECKLIST.md, plus new reports/M3-verifier.md, M3-perf.md, M3-acceptance-matrix.md,
  M3-soak-samples.csv.
- Corroborating evidence cited: reports/M3-verifier.md (VERDICT VERIFIED).

---

## Iron Rule 1 - Fail-closed is the product; NOT user-disableable - PASS

CRITICAL for M3 (this milestone touches the settings UI). Audited exhaustively.

Settings surface is exactly two properties (src/filter.c filter_get_properties, lines 136-150):
- obs_properties_add_bool(props, enabled, ...) at line 141.
- obs_properties_add_text(props, blocklist, ..., OBS_TEXT_MULTILINE) at line 147, with a
  long-description hint at line 148.
There is NO debug / kill / developer / disable-fail-closed / disable-blackout property. The M2
developer group and the debug_kill bool were removed (the diff deletes the developer group add).

- enabled is the ONLY masking-off switch, and it turns the WHOLE filter off (pass-through); it does
  not weaken the blackout: src/filter.c lines 284-289 call obs_source_skip_video_filter when not
  enabled. While enabled, nothing can suppress fail-closed.
- The 500ms threshold is a compile-time constant unreachable from settings: SS_HEARTBEAT_STALE_NS =
  500000000ULL via a define at src/filter.c line 36, commented as deliberately not exposed anywhere
  user-configurable. No obs_data_get call reads it.
- All six uncertainty paths still land in full black via draw_fail_closed():
  (1) no snapshot (have_snap false) and (2) stale heartbeat (age over SS_HEARTBEAT_STALE_NS) go
      unhealthy then draw_fail_closed at filter.c line 351.
  (3) capture geometry unresolved (ss_resolve_capture_geom false) goes unhealthy at line 314 then 351.
  (4) coordinate mapping SS_MAP_INVALID goes unhealthy at line 326 then 351.
  (5) filter-begin bypass (obs_source_process_filter_begin false) draws fail-closed at line 357.
  (6) plate texture allocation failure (tex NULL) draws fail-closed at line 368.
  The newly-inserted STREAMSENTRY_PERF_LOG block (filter.c lines 334-348) sits AFTER the mapping loop
  and BEFORE the unhealthy branch; it only reads a clock and accumulates. It does not touch unhealthy,
  reason, or control flow. The fail-closed decision is unchanged.

ss_watcher_debug_set_killed fault-injection hook: still defined (src/watcher.cpp line 473) and declared
(src/watcher.h line 53), and it can ONLY freeze the heartbeat (force fail-closed) - no path weakens the
blackout. Confirmed test-only: repo-wide grep shows its only callers are tests/watcher-selftest.cpp
lines 173 and 178. It is NOT referenced in src/filter.c or plugin-main.c and is wired to no OBS setting.
The M2 filter.c code that routed a debug_kill setting into it (and the filter_destroy un-kill) was
removed in this diff. Acceptable: a developer/test hook, not a shipping control. The OPT:REF drop is
real - cmake/windows/compilerconfig.cmake lines 53-54 apply /OPT:REF and /OPT:ICF for all non-Debug
configs, so the unreferenced function and its log string are dropped from the release DLL (M3-verifier
Check 4 confirms the DEBUG-kill string is absent from streamsentry.dll yet present in
watcher-selftest.exe).

## Iron Rule 2 - Deterministic only; no ML/AI - PASS

No ML/AI added. Detection remains Win32 EnumWindows + string signature matching + UIA IsPassword and
BoundingRectangle (src/watcher.cpp). Grep of src/ for machine-learn / neural / tensorflow / onnx: no
matches. The perf blocks are pure timing arithmetic (os_gettime_ns deltas).

## Iron Rule 3 - Opaque masking; no blur/pixelate/mosaic; no reversible obfuscation - PASS

Plate/card generation (plate-gen) is UNCHANGED in this diff. No masking code was touched. Grep of src/
for blur, pixelate, mosaic, gaussian, deblur, depixel, downscale: no matches (corroborated M3-verifier
Check 6). Raw black remains reserved for fail-closed (draw_fail_closed). M3 introduces no new drawing.

## Iron Rule 4 - No third-party dependencies beyond obs-plugintemplate - PASS

The perf instrumentation uses only os_gettime_ns (libobs util/platform) and obs_log (plugin logging),
both already declared/used pre-M3 (src/watcher.cpp lines 32-33). No new include of any third-party
library; CMakeLists.txt adds only an option() plus target_compile_definitions on the existing target
(lines 61-68) and links no new library. The DLL import set is byte-for-byte identical to M2 (M3-verifier
Check 7: obs.dll, dwmapi, ole32, USER32, w32-pthreads, KERNEL32, MSVCP140, VCRUNTIME140 family,
api-ms-win-crt family). No Qt, no non-Windows/non-libobs library. ss_watcher_default_blocklist_text uses
WideCharToMultiByte (Win32) only.

## Iron Rule 5 - Scope locked to v0.1 - PASS

- The Blocklist multi-line textbox IS the SPEC Settings UI item - in scope.
- No out-of-scope creep in src/: grep for allowlist, focus assist, tray, auto-update, per-app, macos,
  .env, api-key, secret-scan returns no matches. No allowlist mode, no Focus Assist integration, no
  tray, no auto-update, no content/secret scanning, no macOS, no per-app policy beyond the flat
  blocklist.
- The developer debug toggle was REMOVED as M3 required (locale DebugGroup/DebugKill deleted; the
  filter.c developer group and debug_kill field/default/update deleted).
- Perf logging is dev instrumentation, not a shipping feature: gated behind
  option(STREAMSENTRY_PERF_LOG ... OFF) (CMakeLists.txt line 64), default OFF, and compiled out of the
  shipping DLL (the ifdef guards in filter.c and watcher.cpp; M3-verifier Check 2 confirms
  STREAMSENTRY_PERF_LOG=OFF in the configured build and both PERF strings absent from streamsentry.dll,
  deployed==built SHA256 9835ca17...).

## Iron Rule 6 - GPLv2-or-later, original code - PASS

Both modified source files retain the GPL-2.0-or-later header (src/filter.c lines 1-17; src/watcher.cpp
lines 1-17; src/watcher.h lines 1-17 - headers untouched by the diff). New code
(ss_watcher_default_blocklist_text, the two perf blocks, the blocklist wiring in filter_update) is
straightforward original code written against Win32/libobs APIs. No evidence of imported or adapted
third-party code.

## Architecture conformance - PASS (with one documentation observation)

- One COM MTA watcher thread, 150ms cadence: unchanged (watcher.cpp WATCH_TICK_MS = 150 line 58;
  CoInitializeEx MULTITHREADED line 291). Within the SPEC 100-200ms window.
- Minimal event handlers: the UIA FocusHandler still copies IsPassword and BoundingRectangle under a
  small lock and returns (watcher.cpp lines 228-250). No new work added to handlers.
- Shared state = rect list + heartbeat: unchanged. Render side still uses a non-blocking try-read
  (ss_state_try_read, filter.c line 296) and a tiny critical section.
- Blocklist is global to the single shared watcher (per-filter textbox, last-writer-wins): a reasonable
  v0.1 interpretation of the single-watcher architecture. Documented at the point of behavior (filter.c
  filter_update comment, lines 95-97) and surfaced to the human (HUMAN_CHECKLIST.md item 10).
  OBSERVATION (non-blocking): there is no CHANGELOG entry for M3 yet - the top CHANGELOG section is
  0.1.0-m2, which still says the blocklist textbox is deferred to M3. So the documented-in-CHANGELOG or
  decisions expectation currently rests on the inline code comment plus the human checklist, not on a
  CHANGELOG/decisions entry. This is a documentation-completeness gap, not an iron-rule violation.

## SPEC Settings UI (minimal) compliance - PASS

SPEC.md lines 29-32 require exactly: Enable/disable checkbox; Blocklist multi-line text box (one process
name or title substring per line); no option to disable fail-closed. The implementation matches exactly:
an enabled bool plus a blocklist OBS_TEXT_MULTILINE box, hint text stating one process name or
window-title substring per line, and no fail-closed-disable control. Empty text falls back to the
built-in defaults (filter.c lines 98-99 into watcher.cpp ss_watcher_set_blocklist lines 423-427 and
449-451), which weakens nothing.

## Hardening-evidence honesty assessment - HONEST (acceptable disclosure)

- Perf numbers: watcher about 0.55% of one core (samples 0.803-0.826 ms/tick at 150ms; M3-perf.md),
  under the SPEC under-1%-of-one-core target. Render decision about 0.03 us/frame (about 30 ns). These
  come from a separate PERF_LOG=ON build; M3-perf.md states this plainly. The instrumentation excludes
  GPU draw submission and says so - an honest, correctly-scoped measurement of the work StreamSentry
  itself adds.
- Soak: a 30-minute run vs the SPEC 2-hour target. This is a SCOPE SHORTFALL against the literal SPEC
  number, but it is disclosed honestly and repeatedly (the M3-perf.md caveat that the SPEC endurance
  target is 2 hours and this was 30 min; acceptance-matrix row 9 marked AUTO (30 min) + HUMAN (2 h);
  HUMAN_CHECKLIST item 9 routes the full 2-hour confirmation to the human). It is not passed off as the
  full target. The raw CSV (reports/M3-soak-samples.csv, 60 samples from 40s to 1811s) corroborates the
  flat conclusion: working set 345.3 MB at the 40s startup sample, settling to about 320-321 MB and flat
  to the end; private bytes about 356.0-356.8 flat; handles 6351-6382; threads 234-242 - no monotonic
  growth. The numbers in M3-perf.md are not overstated relative to the CSV.
- Acceptance-matrix classifications (reports/M3-acceptance-matrix.md): honest. Only rows 7 (fault
  injection) and 10 (pass-through) are marked AUTO fully machine-verified, and both carry real evidence
  (watcher-selftest kill/resume plus the M2 OBS integration RUN B for row 7; the soak steady-state
  no-plates and no-fail-closed pass-through for row 10). Rows needing a live trigger (1,2,3,4) are
  MECHANISM/HUMAN, the coordinate rows (5,6) are UNIT/HUMAN, and the longer perf/visual targets (8,9)
  are split AUTO (cost / 30 min) + HUMAN (visual / 2 h). Nothing marked verified lacks evidence;
  MECHANISM is used correctly (code path proven, real-world trigger deferred).

Verdict on honesty: the disclosures are candid and the shortfall (30-min vs 2-hour soak) is explicitly
carried to the human checklist rather than hidden. Acceptable for an M3 hardening milestone; the 2-hour
run remains an open human-checklist item, not a claimed pass.

---

## QUESTIONS for the human (non-blocking)

- Q1 (carried from M2, not introduced by M3): blocklist match semantics are process-name-OR-title-
  substring (M2-DECISIONS.md; HUMAN_CHECKLIST item 10) whereas CLAUDE.md Architecture literally says
  process name AND window class. The OR choice is the iron-rule-1-safe (never-under-mask) reading of
  SPEC Detection section 2 and was already flagged for your ruling at M2. It remains unresolved. This is
  not an M3 regression; noted so it is not lost.
- Q2 (documentation only): there is no CHANGELOG M3 entry yet; the global / last-writer-wins blocklist
  decision and the debug-scaffolding removal are documented in code comments plus HUMAN_CHECKLIST but
  not in CHANGELOG.md. Recommend adding an M3 CHANGELOG section before tagging. Does not block the
  milestone.

RESULT: PASS
