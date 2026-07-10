# TESTING.md — Manual Test Log

Per CLAUDE.md (Workflow), every feature lands with a manual test note here.
Automated evidence lives in `reports/`; this file records what a human must
verify inside OBS Studio and whether it has actually been done.

---

## M0 — 2026-07-05 — Skeleton pass-through filter

### What was built
- Video filter "obsplugin" (source id `obsplugin_filter`), built from
  obs-plugintemplate.
- Pass-through only: the render callback calls
  `obs_source_skip_video_filter()`; nothing is drawn.
- Exactly one property: "Enable" checkbox, default checked.
- No detection, no masking, no fail-closed rendering — all of that is M1+.

### Automated evidence
Sources: `reports/M0-verifier.md` (VERDICT: VERIFIED) and
`reports/M0-spec-guardian.md` (RESULT: PASS), both 2026-07-05.
- Build green: `cmake --build --preset windows-x64-local` exited 0 with zero
  warnings; `build_x64/RelWithDebInfo/obsplugin.dll` produced.
- Import table is libobs-only: `obs.dll` plus VCRUNTIME /
  api-ms-win-crt* / KERNEL32. No Qt, no third-party DLLs.
- `obs_module_load` is exported (standard 7-export OBS module ABI).
- SHA256 of the built DLL and of `data/locale/en-US.ini` match the deployed
  copies under `D:\software\obs\obs-studio`.

### Manual acceptance procedure — STATUS: PASSED (2026-07-05)
Project owner visually confirmed "obsplugin" in the Effect Filters "+" list
(zh-TW UI, screenshot provided in session). Step 4 log evidence was captured
the same day: `[obsplugin] plugin loaded successfully (version 0.1.0)` and
`obsplugin.dll` under "Loaded Modules" in log `2026-07-05 02-20-39.txt`.
Known cosmetic note: `Failed to load 'zh-TW' text for module: 'obsplugin.dll'`
— harmless; localization is out of scope for v0.1 per SPEC.md, en-US fallback
shows the same name.

1. Start OBS Studio: `D:\software\obs\obs-studio\bin\64bit\obs64.exe`.
2. Right-click any video source (e.g. Display Capture) → Filters → under
   "Effect Filters" press "+" → an entry named "obsplugin" must appear;
   add it.
3. The filter's properties must show exactly one checkbox, "Enable"
   (default checked), and the video must look completely unchanged
   (pass-through).
4. Open the newest log in `%APPDATA%\obs-studio\logs\`: it must contain
   `[obsplugin] plugin loaded successfully (version 0.1.0)`, and
   `obsplugin.dll` must be listed under "Loaded Modules".

M0 completion definition: the human has seen the filter in OBS's filter list.

### Not applicable at M0
The manual in-OBS test matrix from SPEC.md (toast masking, blocklist windows,
password-field guard, fail-closed blackout) covers behavior that does not
exist yet; it applies from M1 onward.

---

## M1 — 2026-07-05 — Render path (fake rects)

### What was built
- Mask-render pipeline driven by hardcoded fake rects (real detection is M2).
- Pure coordinate-mapping module (`src/coord-map.c`): screen → source mapping
  with multi-monitor offsets/negative origins, per-capture mixed-DPI ratios,
  scaled/cropped captures, over-mask padding clamped to source bounds; returns
  `SS_MAP_INVALID` on bad geometry so the caller fails closed.
- Pure plate generator (`src/plate-gen.c`): opaque notification card (bell +
  "Notification hidden") for toast rects, opaque privacy plate (lock + "Hidden")
  for window/field rects, opaque fail-closed status banner. Original 5x7 bitmap
  font, no third-party assets.
- Shared state (`src/shared-state.c`): rect list + heartbeat, non-blocking
  trylock reader.
- Filter render path (`src/filter.c`): plates when healthy; full opaque black +
  status banner when unhealthy (six triggers: heartbeat > 500 ms, missing
  snapshot, invalid geometry, mapping failure, filter-begin bypass, texture
  alloc failure). The 500 ms threshold is a compile-time constant, not
  user-settable. Two developer-only DEBUG toggles (fake rects, simulate stall)
  are M1 scaffolding for M3 removal; neither can disable fail-closed.

### Automated evidence
Source: `reports/M1-verifier.md` (VERDICT: VERIFIED) and
`reports/M1-spec-guardian.md` (RESULT: PASS), both 2026-07-05.
- Build green: `cmake --build --preset windows-x64-local` exited 0 with zero
  warnings (grep for warning/error in the build log returned no matches).
- ctest 2/2 suites passed: `coord-map-tests` (11 coordinate-mapping cases) and
  `plate-gen-tests` (plate opacity — every pixel inside the corner inset
  alpha == 255, banner fully opaque, corner inset 5 < pad 12).
- `dumpbin /dependents` on `streamsentry.dll`: `obs.dll`, `w32-pthreads.dll`,
  and VCRUNTIME / api-ms-win-crt* / KERNEL32 only. No Qt, no other DLLs.
- SHA256 of the built DLL matches the deployed copy under
  `D:\software\obs\obs-studio`
  (`a86b062858c5b353d2b4e9cd95887a12c406e2142edcd4d6b56ef7a200cd4e4b`).
- In-OBS log (`2026-07-05 16-47-18.txt`) shows, in order,
  `[streamsentry] mask plates active: 2` (16:47:20.006) then
  `[streamsentry] FAIL-CLOSED engaged: heartbeat stale (heartbeat age 519 ms)`
  (16:47:20.523) — a 517 ms delta.

### Manual acceptance — STATUS: PENDING
The on-screen visual confirmation is in `reports/HUMAN_CHECKLIST.md` (M1
section) — steps are not duplicated here; run them there. The human has not yet
visually confirmed plate appearance on screen (toast card, privacy plate, full
opacity, and the black + red banner on simulated stall).

Caveat (from `reports/M1-verifier.md` Check 6): the OBS log evidence above came
from a build one revision older than the committed M1. The only difference is a
log-wording fix in `log_mode`; the fail-closed mechanism is unchanged. The exact
current-tree binary has not itself been observed inside OBS, so the on-screen
visual check remains outstanding.

### Not yet covered by automation
Per `reports/M1-verifier.md`, `src/filter.c` and `src/shared-state.c` have no
unit tests (render/tick logic depends on libobs graphics) — verified by source
audit plus the in-OBS log. Real detection (window enumeration / UIA / real
multi-monitor DPI) is M2; the mixed-DPI and multi-monitor cases exist here only
as pure-math unit tests. Performance (60fps render cost), the 2-hour leak run,
and the full SPEC.md acceptance matrix remain manual and are not part of M1
verification.

### Manual in-OBS test matrix from SPEC.md — still outstanding
Toast masking, blocklist windows, and the password-field guard on real detected
windows are M2 behavior and cannot be exercised yet. What M1 adds to the human's
eyeball pass is the visual quality and opacity of the plates and the fail-closed
blackout, tracked in `reports/HUMAN_CHECKLIST.md` (M1) and PENDING above.

---

## M2 — 2026-07-05 — Real detection (watcher thread)

### What was built
- Detection watcher (`src/watcher.cpp`): one COM MTA thread, refcounted across
  filter instances, enumerating windows on a 150 ms timer and publishing real
  screen-space rects plus a heartbeat into shared state. Replaces M1's fake
  rects.
- Toast detection: process-AND-class signature, determined empirically here as
  `explorer.exe` + `Xaml_WindowedPopupClass` (Windows 11 build 26200), gated by
  DWMWA_CLOAKED + visible + non-empty rect.
- Blocklist detection: process-name-OR-title-substring match; built-in defaults
  are password managers + credential dialogs. The settings textbox to edit it is
  M3.
- Password-field detection: a UIA focus-changed handler reads
  `UIA_IsPasswordProperty` and reports the field's `BoundingRectangle`; the
  handler does minimal work.
- Filter-side capture-geometry resolution (`src/geom-resolve.c`): display/monitor
  capture only, and only when exactly one monitor matches the source base size;
  ambiguous or unsupported sources fail closed.
- `src/filter.c` rewired to map the real rects through the M1 coordinate module;
  startup is fail-closed (black) until the watcher's first heartbeat. M1's
  fake-rect toggles were replaced by a `debug_kill` toggle that freezes the
  heartbeat to force fail-closed and cannot disable it.

### Automated evidence
Sources: `reports/M2-verifier.md` (VERDICT: VERIFIED) and
`reports/M2-spec-guardian.md` (RESULT: PASS), both 2026-07-05.
- Build green: `cmake --build --preset windows-x64-local` exited 0 with zero
  warnings; a clean full recompile compiled every M2 source and linked
  `streamsentry.dll`.
- ctest 2/2 pure suites passed: `coord-map-tests` and `plate-gen-tests`
  (`ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure`,
  exit 0). `plate-gen.c/.h` and `coord-map.c/.h` are byte-unchanged vs M1.
- `dumpbin /dependents` on `streamsentry.dll`: `obs.dll`, `dwmapi.dll`,
  `ole32.dll`, `USER32.dll`, `w32-pthreads.dll`, `KERNEL32.dll`, `MSVCP140.dll`,
  `VCRUNTIME140_1.dll`, `VCRUNTIME140.dll`, and `api-ms-win-crt-*` — Windows
  COM/DWM + the C++ runtime + libobs only. No Qt, no third-party DLL.
- The deployed DLL under `D:\software\obs\obs-studio` matches the pinned SHA256
  exactly (`C010F3CC5A5498C939881A40FBE21833CEDE411FBBAB0C915AD6544C5E333442`).
  Note: the build-tree DLL was recompiled during verification, so its hash
  differs (RelWithDebInfo embeds a fresh PDB signature); its dependents are
  identical to the deployed copy.
- Watcher self-test (`reports/M2-watcher-selftest-output.txt`, exit 0): watcher
  thread starts and heartbeat advances; a blocklist rect appears when notepad
  opens and disappears when it closes; the heartbeat freezes for >500 ms under
  fault injection (render fails closed) and resumes on release; the thread stops
  cleanly.
- In-OBS integration (`reports/M2-obs-integration.txt`): RUN A (healthy) starts
  FAIL-CLOSED at load, then clears to normal rendering ~140 ms later once the
  heartbeat is fresh; RUN B (`debug_kill=true`) stays fail-closed for the whole
  run (no "cleared" line) with the heartbeat frozen. Both end with a clean
  watcher shutdown. OBS was launched only for these two documented runs.

### Manual acceptance — STATUS: PENDING
The on-screen confirmation is in `reports/HUMAN_CHECKLIST.md` (M2 section);
steps are not duplicated here. The human has not yet visually confirmed the
password-field privacy plate, the toast card, the blocklist-window plate,
plate opacity, and the fail-closed blackout on screen.

Verification gaps carried from the reports (both auditors flagged these):
- The automated toast test was INCONCLUSIVE: this machine has Do Not Disturb /
  Focus Assist active, which routes toasts to the notification center with a
  0x0 rect, so no on-screen banner ever appeared. Turn Do Not Disturb OFF
  before the manual toast check.
- Live password-field-to-plate masking was NOT machine-verified: the self-test
  exercises the UIA focus-changed path structurally (handler registration) but
  does not focus a real password field, so the field-rect-to-plate result is a
  manual-matrix item.
- The blocklist match-semantics decision (process-name-OR-title-substring vs
  CLAUDE.md's "process AND class") is documented in `reports/M2-DECISIONS.md`
  and awaits the owner's ruling (see `reports/HUMAN_CHECKLIST.md`).

### Not yet covered by automation
Per `reports/M2-verifier.md`: real on-screen toast masking (DND-blocked here),
end-to-end UIA password-field masking, coordinate landing accuracy on a second
monitor with different DPI and on scaled/cropped sources, and the blocklist
settings-textbox wiring (deferred to M3) are not machine-verified. Performance
(watcher CPU cost) and the 2-hour idle leak run remain SPEC manual-matrix items.

### Manual in-OBS test matrix from SPEC.md — still outstanding
M2 makes the SPEC acceptance behaviors exercisable for the first time: toast
masking (needs DND off), blocklist-window masking, and the password-field
guard on real detected windows, plus the fail-closed blackout. All are tracked
in `reports/HUMAN_CHECKLIST.md` (M2) and PENDING above. `src/watcher.cpp`,
`src/geom-resolve.c`, and the M2 render wiring in `src/filter.c` have no unit
tests (they depend on live OS window state / libobs graphics) and rest on the
self-test plus the in-OBS logs above.

---

## M3 — 2026-07-05 — Settings UI + hardening

### What was built
- Settings UI (`src/filter.c`): the SPEC "minimal" settings — an **Enable**
  checkbox and a multi-line **Blocklist** text box (one process name or
  window-title substring per line), pre-filled with the built-in defaults via
  `ss_watcher_default_blocklist_text()`. Deliberately **no** option to disable
  fail-closed.
- The blocklist is global to the single shared watcher thread (per-filter
  textbox, last-writer-wins) — a documented v0.1 interpretation.
- Removed the developer debug scaffolding (the `debug_kill` "freeze watcher
  heartbeat" toggle) from the shipping UI. The underlying
  `ss_watcher_debug_set_killed` hook remains for the standalone self-test only,
  wired to no setting, and is dropped from the release DLL by `/OPT:REF`.
- Optional performance instrumentation behind the CMake option
  `STREAMSENTRY_PERF_LOG` (OFF by default, compiled out of the shipping build).

### Automated evidence
Sources: `reports/M3-verifier.md` (VERDICT: VERIFIED) and
`reports/M3-spec-guardian.md` (RESULT: PASS), both 2026-07-05.
- Build green: `cmake --build --preset windows-x64-local` exited 0 with no
  warnings in the incremental build tail.
- ctest 2/2 pure suites passed: `coord-map-tests` and `plate-gen-tests`
  (`ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure`, exit 0).
- Watcher self-test (`watcher-selftest.exe`) exited 0: thread started; heartbeat
  advanced; a blocklist rect appeared after notepad opened and disappeared after
  it closed; the heartbeat froze >500 ms under fault injection (render fails
  closed) and resumed after release; clean shutdown. The toast line was
  INCONCLUSIVE (machine Do Not Disturb) — accepted.
- Perf compiled OUT of the shipping DLL: `CMakeCache.txt` has
  `STREAMSENTRY_PERF_LOG=OFF`; neither `PERF watcher tick` nor
  `PERF render decision` is present in `streamsentry.dll`. The built and deployed
  DLL share SHA256 `9835ca172898c46672e7eb9e08e7953be107696797a93a26584b5c998ef0cf12`,
  matching the pinned value.
- Dependencies unchanged vs M2 and no Qt: `dumpbin /DEPENDENTS` on both the built
  and deployed DLL returns the identical import set (`obs.dll`, `dwmapi.dll`,
  `ole32.dll`, `USER32.dll`, `w32-pthreads.dll`, `KERNEL32.dll`, `MSVCP140.dll`,
  `VCRUNTIME140_1.dll`, `VCRUNTIME140.dll`, `api-ms-win-crt-*`).
- Performance and soak (`reports/M3-perf.md`, raw data
  `reports/M3-soak-samples.csv`, from a separate `STREAMSENTRY_PERF_LOG=ON`
  build): watcher tick ~0.55% of one core (0.803–0.826 ms/tick at 150 ms cadence)
  under the SPEC `< 1% of one core` target; render decision avg 0.03 µs/frame
  (~30 ns); 30-minute soak stable with no leak indicators (working set 345 MB at
  startup, ~320–321 MB flat for the remaining 27 min; private bytes ~356.5 MB
  flat; handles 6351–6382 and threads 234–242 with no monotonic growth).
- Acceptance matrix (`reports/M3-acceptance-matrix.md`): all 10 SPEC acceptance
  rows are mapped to evidence; rows 7 (fault injection) and 10 (empty
  blocklist / no password field → pass-through) are fully machine-verified. The
  remaining rows are classified MECHANISM / UNIT / HUMAN with the longer SPEC
  targets (2-hour soak) routed to the human checklist.

### Manual acceptance — STATUS: PENDING
The on-screen / real-device confirmations are in `reports/HUMAN_CHECKLIST.md`;
steps are not duplicated here. Carried items still outstanding:
- Settings-UI visual check: the Enable checkbox and the Blocklist textbox
  pre-filled with the defaults render as expected in the OBS properties dialog,
  and editing a line takes effect (checklist item 1). OBS was not launched during
  M3 verification.
- Real toast masking (needs Do Not Disturb OFF), password-field masking, and
  blocklist-window masking on real detected windows (items 2–5).
- Coordinate accuracy on real multi-monitor / mixed-DPI / scaled captures
  (items 6, 7).
- The full 2-hour soak — only a 30-minute soak was measured this session
  (item 9).
- The still-open blocklist-semantics decision: process-name-OR-title-substring
  (as implemented) vs CLAUDE.md's literal "process AND class", awaiting the
  owner's ruling (item 10; `reports/M2-DECISIONS.md`).

### Not covered by automation
Per `reports/M3-verifier.md`: real on-screen toast masking (DND-blocked here),
end-to-end UIA password-field masking, credential-dialog / UAC (secure desktop,
not capturable), live multi-monitor / mixed-DPI / scaled-source landing accuracy,
the visual "no dropped frames" during recording, the full 2-hour soak, and the
in-OBS rendering of the settings dialog were not machine-verified this session
(OBS was not launched). `src/filter.c`'s render/settings wiring has no unit test;
the perf/soak numbers are read from a separate `STREAMSENTRY_PERF_LOG=ON` build
and were not re-run this session.

### Manual in-OBS test matrix from SPEC.md — still outstanding
The full SPEC acceptance matrix is mapped in `reports/M3-acceptance-matrix.md`.
Machine-verified: rows 7 and 10. The human's ~10-minute eyeball pass must still
cover the settings UI as rendered, real toast / password-field / blocklist-window
masking and plate opacity on screen, coordinate accuracy on real
multi-monitor / DPI / scaled captures, the live fail-closed blackout, and the
full 2-hour soak — all tracked in `reports/HUMAN_CHECKLIST.md` and PENDING above.

---

## M4 — 2026-07-05 — Ship-ready (docs + packaging)

M4 changed no code logic — it is the final documentation and release-packaging
pass. The shipping DLL is unchanged; only README.md and the `reports/` were
touched (plus the packaging dry-run under the git-ignored `release/`).

### Automated evidence
Sources: `reports/M4-verifier.md` (VERDICT: VERIFIED, 7/7 checks) and
`reports/M4-spec-guardian.md` (RESULT: PASS), both 2026-07-05.
- CI-parity build with warnings-as-errors
  (`CMAKE_COMPILE_WARNING_AS_ERROR=ON`): a forced clean full recompile of every
  translation unit exited 0 with zero warning/error lines.
- ctest 2/2 pure suites passed (`coord-map-tests`, `plate-gen-tests`), and
  `watcher-selftest.exe` exited 0 (blocklist rect appear/disappear, heartbeat
  freeze >500 ms / resume, clean start/stop; toast line INCONCLUSIVE under
  machine Do Not Disturb — accepted).
- Shipping-DLL hygiene: `streamsentry.dll` has performance instrumentation
  compiled out (no `PERF watcher tick` string) and `dumpbin /dependents` shows
  libobs + Windows COM/DWM/pthreads + the C++ runtime only — no Qt, no
  third-party dependency.
- Release-zip layout verified via install dry-run:
  `streamsentry-0.1.0-windows-x64.zip` contains exactly
  `streamsentry/bin/64bit/streamsentry.dll` (+ `.pdb`) and
  `streamsentry/data/locale/en-US.ini`, matching the README download
  instructions; `release/` is git-ignored and absent from `git status`.
- README honesty (`reports/M4-spec-guardian.md`): PASS — no overclaims, and all
  v0.1 limitations are disclosed.

`reports/FINAL.md` is the consolidated index — machine-verified evidence, the
remaining human checklist, and the exact human publish/tag steps — and
`reports/HUMAN_CHECKLIST.md` holds the remaining manual pass.

### Manual acceptance (overall v0.1) — STATUS: PENDING
The v0.1 manual acceptance is not yet done. It still requires the human pass in
`reports/HUMAN_CHECKLIST.md` (on-screen plate/card opacity; real toast masking
with Do Not Disturb OFF; live password-field and blocklist-app masking;
coordinate accuracy on a real second monitor / different DPI and a
scaled/cropped source), the optional full 2-hour endurance soak (only 30 min was
measured, in M3), and the owner's ruling on the blocklist match-semantics
decision (process-name-OR-title-substring vs CLAUDE.md's literal "process AND
class"; `reports/M2-DECISIONS.md`).

---

## Post-M0 — 2026-07-05 — Renamed obsplugin → StreamSentry

No behavior change; identity-only rename (see CHANGELOG.md for the full
list of renamed identifiers). Re-verified directly (not re-run through the
verifier/spec-guardian agents — no logic changed, so the M0 gate evidence
still applies to the code paths themselves):
- Rebuilt clean (deleted `build_x64`, reconfigured, rebuilt) since
  `buildspec.json` is not a tracked CMake configure dependency and a stale
  cache would have kept producing `obsplugin.dll`.
- `dumpbin /dependents` on the new `streamsentry.dll`: still libobs-only
  (`obs.dll` + VCRUNTIME/api-ms-win-crt*/KERNEL32), no Qt.
- Old deployed artifacts (`obsplugin.dll`/`.pdb`,
  `data\obs-plugins\obsplugin\`) removed from
  `D:\software\obs\obs-studio`; new ones deployed under the `streamsentry`
  name.
- OBS log confirms `[streamsentry] plugin loaded successfully (version
  0.1.0)`; no `obsplugin` references remain in the log.

### Manual re-confirmation — STATUS: PENDING
The filter's on-screen name changed (was "obsplugin", now "StreamSentry").
Ask the owner for one more glance: Filters → Effect Filters "+" → confirm
the entry now reads **StreamSentry**.

### Directory note
`F:\obsplugin` (this repo's original path) could not be renamed or removed
from within the Claude Code session that performed this change — it is
that session's protected primary working directory. A verified full mirror
(git history, working tree, `.deps`) was made via `robocopy` to
`F:\StreamSentry`. Treat `F:\StreamSentry` as canonical going forward;
`F:\obsplugin` can be deleted manually once confirmed redundant.

---

## M5 — 2026-07-06 — v0.2 kickoff (docs + governance)

M5 changed no code and no behavior — it opens the v0.2 cycle with
documentation and governance only. The change set is exactly four files:
`ARCHITECTURE.md` (new: as-built v0.1 record — module map, thread model,
fail-closed state machine with trigger table, data flow, render-side plate
caching, test infrastructure, known architectural limits), `SPEC.md`
(retitled to cover both versions; Part 1 kept as the shipped v0.1 baseline
plus one inline as-built note; new Part 2 = the owner-approved v0.2 items,
a v0.2 out-of-scope list, and the v0.2 acceptance-matrix additions),
`CLAUDE.md` (iron rule 5 scope lock v0.1 → v0.2; matching line aligned with
owner ruling a434b18), and `.gitignore` (whitelists ARCHITECTURE.md). No
`src/`, `tests/`, or `CMakeLists.txt` changes; no version bump.

### Automated evidence
Sources: `reports/M5-verifier.md` (FINAL VERDICT: VERIFIED, 5/5 checks) and
`reports/M5-spec-guardian.md` (Verdict: PASS), both 2026-07-06.
- Change-set containment: `git status --short`, run before AND after the
  build, shows exactly the four staged doc files — no entry for `src/`,
  `tests/`, or `CMakeLists.txt`.
- From-scratch build green: `build_x64` was deleted first, then
  `cmake --preset windows-x64-local` (exit 0) and
  `cmake --build --preset windows-x64-local` (RelWithDebInfo, exit 0)
  recompiled every translation unit. A warning grep over the full build log
  (`warning|error|fatal|C4\d{3}|C5\d{3}|LNK\d+`, plus localized zh-TW
  variants) found zero matches. Verifier caveat: unlike M4, this run did not
  set `CMAKE_COMPILE_WARNING_AS_ERROR`, so the zero-warning claim rests on
  that log grep of a full clean recompile.
- Automated tests: `ctest --test-dir build_x64 -C RelWithDebInfo
  --output-on-failure` exit 0, 2/2 suites; the executables were also run
  directly — `coord-map-tests` 11/11 and `plate-gen-tests` 7/7 (18/18
  cases).
- Artifacts present: `streamsentry.dll` + `.pdb` (DLL byte size identical
  to the M4-verified DLL), both test executables, and
  `watcher-selftest.exe`. The self-test was built but intentionally NOT run
  this milestone (it is manual-only: it launches notepad and fires a real
  toast).
- Document-content audit (spec-guardian): all six iron rules present, five
  byte-identical; rule 5's v0.1 → v0.2 bump is the one owner-approved change
  (per `reports/V02-PLAN.md`). SPEC Part 2 maps 1:1 onto the approved plan
  (2.1/2.2 → M6, 2.3/2.4 → M7, 2.5 → M8; 2.6 Chromium investigation is
  documentation-only, non-gating) with nothing beyond it. CLAUDE.md's
  matching line agrees with the FINAL ruling a434b18 AND with the as-built
  code in `src/watcher.cpp`. ARCHITECTURE.md was spot-checked
  claim-by-claim against `src/` — no materially false claim found.

### Manual acceptance — STATUS: NOT REQUIRED (no behavior change)
Nothing in M5 can change what a user sees in OBS, so M5 adds no new manual
in-OBS steps; OBS was not launched during verification. The manual-matrix
additions for the v0.2 features (watcher perf hardening, toast geometry
narrowing, allowlist mode, panic hotkey, window-picker UI) arrive with
M6–M8 — SPEC.md Part 2 already carries the v0.2 acceptance rows those
milestones must satisfy.

### Manual in-OBS checklist — current status after M5
Since M4 the owner field-tested v0.1 live (recorded in
`reports/V02-PLAN.md`, session of 2026-07-05), which moves several items:
- Field-validated: SRT streaming works with the filter active; blocklist
  masking works live (log shows up to 27 simultaneous plates); mid-plate
  rendering solid. The settings dialog was used live in that session (the
  owner's UX findings target the blocklist textbox), though no report line
  separately confirms an edited line taking effect; M8's picker rework
  supersedes this dialog.
- RESOLVED: the blocklist match-semantics question. The owner ruling is
  FINAL (commit a434b18): process-name OR title-substring, case-insensitive;
  M5 codifies it in CLAUDE.md. Do not re-ask.
- Field findings feeding v0.2 (they gate M6/M7, not the human checklist):
  fail-closed heartbeat-stale blips under streaming load (age 501–505 ms,
  clearing ~100–150 ms later, roughly once a minute) and toast
  over-matching (shell flyouts share the XAML class → 9–27 plates
  flapping) → M6; the blocklist cannot protect unanticipated windows →
  M7 allowlist mode.

Still outstanding for the human ~10-minute pass (carried; steps live in
`reports/HUMAN_CHECKLIST.md`):
- A real single notification toast masked as the notification card, with Do
  Not Disturb OFF (the field test surfaced over-match noise, not a
  confirmed single-toast card; M6's geometry narrowing re-opens this row
  with a mandatory real-toast acceptance check anyway).
- End-to-end password-field privacy plate on a real focused field (Chromium
  fields are undetected — documented v0.1 limitation; per-browser
  investigation is non-gating v0.2 item 2.6).
- Coordinate landing accuracy on a second monitor / different DPI and on a
  scaled or cropped capture.
- Full 2-hour endurance soak (30 minutes measured in M3; the approved plan
  re-runs perf + soak in M6).
- Minor: one explicit glance that the filter entry reads **StreamSentry**
  (the field session used the filter live, but no report line records the
  displayed name).

Directory note (supersedes the post-rename note above): per
`reports/V02-PLAN.md`, v0.2 work continues in `F:\obsplugin` (the
session-protected primary working directory); `F:\StreamSentry` is the
robocopy mirror, synced after each milestone.

---

## M6 — 2026-07-09 — v0.2 hardening (watcher perf + toast geometry gate)

### What was built
- Watcher performance hardening (SPEC Part 2 item 2.1; `src/watcher.cpp`):
  - PID→image-name cache keyed by PID. An entry is trusted only while its PID
    is observed in consecutive ticks: absent for one full tick → evicted; a
    reused PID re-queries. Failed lookups are never cached, so process-name
    matching keeps retrying. The residual PID-reuse race (reuse within a
    single tick-to-tick window) is documented in code as bounded;
    title-substring matching is unaffected by it.
  - Always-compiled slow-tick warning: any watcher tick > 250 ms logs a
    warning in every build — a compile-time constant (half the 500 ms
    fail-closed stale threshold) outside every `#ifdef`, not a user setting.
  - `STREAMSENTRY_PERF_LOG` instrumentation upgraded: per-200-tick
    avg/max/p99 tick cost plus cache hit/eviction counts. The CMake option
    stays OFF by default and compiled out of the shipping build.
  - Thread priority deliberately NOT applied (SPEC's measure-first order);
    recorded in a comment at the thread-creation site; zero
    `SetThreadPriority` calls in `src/`.
  - Heartbeat semantics untouched: the heartbeat is still written only with a
    completed publish, and the kill path still freezes both.
- Toast-match narrowing by geometry (SPEC Part 2 item 2.2):
  - New pure module `src/toast-gate.c`/`.h`, applied AFTER the v0.1
    process-AND-class signature: the window must sit in the right-edge spawn
    band of some monitor (right edge within 160 px of the monitor's right
    edge; one-sided, so slide-in overhang passes; vertical position free, so
    top-right and bottom-right anchors both pass) and have plausible banner
    dimensions (width 200–1000 px and ≤ 60% of monitor width; height
    60–1200 px and ≤ 90% of monitor height). Monitors are re-enumerated every
    tick.
  - Gate failure only removes the toast-card classification — the window
    still falls through to block/allowlist matching. Fail-closed health is
    not involved in the gate at all.
  - Uncertainty classifies as toast (mask): NULL/degenerate/non-finite
    monitor or window geometry, or no monitor data at all.
  - Monitor-list truncation guard (spec-guardian first-pass finding, fixed
    in-milestone): if `EnumDisplayMonitors` fails, the list truncates at the
    16-monitor cap, or `GetMonitorInfoW` fails for any monitor, the gate is
    handed an EMPTY monitor list — every signature match is then masked as a
    toast (v0.1-parity over-mask) instead of a toast on an unlisted monitor
    going unmasked.
  - The gate constants are PROVISIONAL — derived from documented Windows
    toast metrics (396-DIP banner) with generous over-mask-safe bounds and
    derivations in code comments — under the OWNER RULING of 2026-07-09
    recorded in `reports/M6-toast-probe.txt`: toast banners are
    system-suppressed on this machine, so no live banner could be captured
    for calibration.
- New ctest suite `toast-gate-tests` (`tests/toast-gate-tests.c`, 23
  assertions): positive toast shapes at 100/150/200% DPI including slide-in
  animation and a negative-origin secondary monitor; negative flyover
  fixtures recorded live in `reports/M6-toast-probe.txt` (the one admitted
  tray-overflow-sized shape is asserted as deliberate over-mask, so future
  tightening is a conscious act); multi-monitor any-semantics; and
  uncertainty-must-mask cases (NULL/zero monitors, degenerate, NaN).
- `CMakeLists.txt`: `toast-gate.c` wired into the plugin, the new ctest
  target, and `watcher-selftest`. `ARCHITECTURE.md` updated as-built.
  `reports/M6-toast-probe.txt` added (probe evidence + owner ruling).

### Automated evidence
Sources: `reports/M6-verifier.md` (FINAL VERDICT: VERIFIED — 7/7 checks; its
ADDENDUM is authoritative: `src/watcher.cpp` was amended mid-verification —
the monitor-truncation guard — so the tree was pinned by blob hash and the
entire from-scratch sequence re-run clean against the final staged tree) and
`reports/M6-spec-guardian.md` (Verdict: PASS on re-audit; the one first-pass
residual was fixed in-milestone and re-verified in the over-mask direction;
zero open code findings; both due M5 observations closed), both 2026-07-09.
- From-scratch build green (`build_x64` deleted first):
  `cmake --preset windows-x64-local` then
  `cmake --build --preset windows-x64-local` (RelWithDebInfo), both exit 0,
  every translation unit recompiled. Warning grep over the full build logs
  (EN + localized zh-TW patterns) → zero matches. Same caveat as M5: the
  preset does not set `CMAKE_COMPILE_WARNING_AS_ERROR`; the zero-warning
  claim rests on that log grep.
- Both `STREAMSENTRY_PERF_LOG` variants compile clean: reconfigure with
  `-DSTREAMSENTRY_PERF_LOG=ON` → build → reconfigure `=OFF` → build, the
  cache value verified in `CMakeCache.txt` at each step; the tree is LEFT in
  the OFF state — the shipping configuration.
- `ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure` exit 0,
  3/3 suites: `coord-map-tests`, `plate-gen-tests`, `toast-gate-tests` (newly
  registered). Each test executable was also run directly: "all passed",
  exit 0 each.
- `watcher-selftest.exe` exit 0 — 8/8 deterministic checks (thread start,
  heartbeat advance, blocklist rect appears/disappears around a notepad
  launch/kill, heartbeat frozen > 500 ms under fault injection → render fails
  closed, resume after release, clean stop). The toast leg is INCONCLUSIVE —
  EXPECTED on this machine: banners are system-suppressed (see item 4 below),
  so no banner window exists to detect.
- Artifacts present in `build_x64/RelWithDebInfo/`: `streamsentry.dll` +
  `.pdb` and all four test executables.
- Deployment note (verifier, informational): a perf-instrumented
  (`PERF_LOG=ON`) `streamsentry.dll` was deployed to
  `D:\software\obs\obs-studio\obs-plugins\64bit\` on 2026-07-09 21:00 for the
  owner's soak measurement. Its provenance was not independently verified by
  the clean run (72704 bytes vs the fresh OFF build's 70144 — consistent with
  the ON variant).

### Not covered by automation (from the verifier)
- No runtime performance sample exists: the PERF_LOG stats are verified
  compile-only; nothing in the run measured tick latency under load.
- The > 250 ms warning was never triggered (no slow-tick fault injection
  exists); PID-cache behavior under real process churn / PID reuse is
  exercised only incidentally (the selftest's notepad), not asserted.
- The watcher-side TRIGGERS of the empty-monitor-list fallback (real
  `EnumDisplayMonitors` failure, > 16 monitors, `GetMonitorInfoW` failure)
  have no automated fixture — verified by inspection and compilation only;
  the gate-level direction (empty list → mask) is unit-tested.
- No real toast banner can currently be exercised on this machine, automated
  or manual; the unit suite covers the gate math against documented metrics
  and recorded negative fixtures only. OBS was not launched; live
  multi-monitor / mixed-DPI accuracy remains unit-math only.

### Manual acceptance — STATUS: PENDING (owner's M6 pass; SUPERSEDED as the current checklist — M6.5 below renames the log lines these items grep for; use the M6.5 checklist)
Items 1–2 and the deferred block in item 4 are binding acceptance rows for
the v0.2 ship.
1. **30-minute streaming soak** with the deployed perf-instrumented DLL
   (SPEC 2.1 acceptance), busy desktop:
   - expect ZERO `FAIL-CLOSED engaged: heartbeat stale` lines in the OBS log
     (the v0.1 field test showed roughly one such blip per minute under
     streaming load); *(SUPERSEDED by M6.5: the line is renamed — grep
     `PROTECTION DEGRADED: heartbeat stale` instead; the old string no
     longer exists in the binary, so a zero-hit grep for it proves
     nothing.)*
   - from the `PERF watcher tick` log lines, record the tick p99 — target
     ≤ 50 ms — and the pid-cache hit rate; record the numbers in `reports/`.
2. **Flyout storm check** (SPEC 2.2 acceptance): open Start search and
   taskbar panels/flyouts with the filter active → no mass "Notification
   hidden" plates. Note: the probe shows the shell window taxonomy changed on
   build 26200.8655 (tray overflow and task switcher now use different window
   classes than the v0.1 field notes), so re-check the original 2026-07-05
   storm sources live rather than assuming them.
3. **Blocklist regression**: a blocklisted window still gets its privacy
   plate (the gate must not have disturbed block/allowlist fall-through).
4. **DEFERRED — blocked, not forgotten** (owner ruling 2026-07-09,
   `reports/M6-toast-probe.txt`): real-toast masking (card up before content
   is readable), empirical calibration of the PROVISIONAL gate constants, and
   re-verification of the v0.1 toast signature on Windows build 26200.8655.
   Blocked until toast banners display on this machine again: banners worked
   in the 2026-07-05 field test but are system-suppressed as of 2026-07-09 —
   every notification goes silently to Notification Center, even from senders
   whose `ToastNotifier.Setting` reports Enabled. These items remain binding
   for the v0.2 ship.
   - Diagnostic note for ALL toast testing (this is the note promised by the
     probe record): banners can be system-suppressed while the machine looks
     normal (notifications ON, Do Not Disturb OFF). A "toast not masked"
     result must first be split into **"banner never displayed"** (machine
     state — check whether the notification landed silently in Notification
     Center; fire a probe notification) vs **"detection missed it"** (product
     bug). Only the second is a failure.

### Manual in-OBS checklist — carried items still open after M6
Unchanged from M5 (steps live in `reports/HUMAN_CHECKLIST.md`): end-to-end
password-field privacy plate on a real focused field (Chromium fields
undetected — documented v0.1 limitation; per-browser investigation is
non-gating v0.2 item 2.6); coordinate landing accuracy on a second monitor /
different DPI and on a scaled or cropped capture; the full 2-hour endurance
soak (30 min measured in M3; item 1 above is the separate SPEC-2.1 streaming
soak); one glance that the filter entry reads **StreamSentry**. The former
"real single toast with Do Not Disturb OFF" row is absorbed into item 4
above (same blocker).

---

## M6.5 — 2026-07-10 — Fail-open repositioning (owner ruling 2026-07-09)

### What changed
M6.5 is a behavior repositioning, not a feature. The owner ruled
(`reports/RULING-2026-07-09-fail-open.md`, FINAL, plus its 2026-07-10
Application addendum): the product is a privacy **assist** — wrong masking is
worse than under-masking, the plugin must never disrupt the user's output,
masks are drawn only on confidence, and protection failures must be told to
the user, never silent. This supersedes the v0.1 fail-closed blackout
everywhere. Under that authority: CLAUDE.md iron rules 1 and 3 were
rewritten; SPEC.md Part 2 gains §2.7 with superseded notes on the affected
Part 1 lines; ARCHITECTURE.md's failure semantics were rewritten and the
multi-agent gate workflow documented; README no longer claims a blackout; the
spec-guardian's own checklist (`.claude/agents/spec-guardian.md`) was amended
(Application addendum item 4).

Behavior, as staged and verified:
- **Full-frame blackout REMOVED.** Any unverified state — stale heartbeat
  (> 500 ms), missing snapshot, unresolved capture geometry, per-rect mapping
  failure, filter-begin bypass — renders the source UNMODIFIED plus a small
  opaque top-left status chip ("StreamSentry: protection degraded - see
  log") and one `PROTECTION DEGRADED: <reason>` LOG_WARNING per engagement;
  clearing logs `protection restored: full masking active again` (INFO). No
  path draws raw black anymore (`gs_clear` absent from `src/`; the old
  full-black fill is gone — guardian §2).
- **Confident masks are never dropped.** Rects that are confidently detected
  AND mapped keep their opaque plates even on degraded frames; plate-texture
  allocation failure falls back to a solid opaque fill at the mapped rect.
- **New pure module `src/frame-decide.c`/`.h`**: the entire per-frame
  decision (no-snapshot/stale → detection-degraded → geometry → per-rect
  mapping, first-trigger reason precedence), extracted from `src/filter.c`
  after guardian finding V6 so the decision is unit-testable; no OBS or
  Windows includes.
- **Watcher (`src/watcher.cpp`)**: the snapshot gains a `detection_degraded`
  flag, published when monitor enumeration fails or truncates (the toast
  gate can then no longer affirm); engage/clear transitions are logged once
  each and the render side shows the chip — no degradation is silent
  (guardian finding V1). A window whose DWMWA_CLOAKED state cannot be
  queried is now treated as cloaked and skipped (SPEC §2.7 direction,
  supersedes v0.1's report-on-doubt). Toast-gate uncertainty
  (missing/incomplete monitor data, degenerate geometry) now classifies as
  NOT a toast — the inversion of M6's over-mask direction, per the ruling.
- Unchanged (guardian §5): blocklist matching semantics, heartbeat
  semantics, the compile-time 500 ms staleness constant, opacity of
  everything drawn, deterministic-only, and the dependency set.

### Automated evidence
Sources: `reports/M6.5-verifier.md` (FINAL VERDICT: VERIFIED — three full
from-scratch runs, one per revision of the evolving tree, each pinned by
blob hash; the RUN-3 ADDENDUM is authoritative for the final staged tree)
and `reports/M6.5-spec-guardian.md` (ROUND-3 VERDICT: PASS on that same
26-file change set), audits completed 2026-07-10.
- Commands (RUN-3, verbatim from the report):
  ```
  rm -rf F:/obsplugin/build_x64
  cmake --preset windows-x64-local
  cmake --build --preset windows-x64-local
  cmake --preset windows-x64-local -DSTREAMSENTRY_PERF_LOG=ON
  cmake --build --preset windows-x64-local
  cmake --preset windows-x64-local -DSTREAMSENTRY_PERF_LOG=OFF
  cmake --build --preset windows-x64-local
  ctest -C RelWithDebInfo --output-on-failure          # cwd: build_x64
  ./coord-map-tests.exe; ./plate-gen-tests.exe; ./toast-gate-tests.exe; ./frame-decide-tests.exe
  ./watcher-selftest.exe                               # cwd: build_x64/RelWithDebInfo
  ```
- Results: every configure/build exit 0 with zero warnings (EN + localized
  zh-TW patterns; both `STREAMSENTRY_PERF_LOG` variants; tree left OFF —
  the shipping configuration). ctest 4/4 suites: `coord-map-tests`,
  `plate-gen-tests`, `toast-gate-tests`, `frame-decide-tests` (new) — 35 +
  24 + 23 + 25 = 107 CHECK assertions; each executable also run directly,
  "all passed", exit 0.
- `watcher-selftest.exe` exit 0, all deterministic checks pass; the kill leg
  now reads "heartbeat frozen while killed (>500ms) -> render shows
  protection-degraded chip". The toast leg is INCONCLUSIVE — expected:
  banners are still system-suppressed on this machine
  (`reports/M6-toast-probe.txt`); not a regression.
- `tests/toast-gate-tests.c`: exactly the five uncertainty CHECKs inverted
  to NOT-toast; the NaN fixture strengthened so the case stays
  discriminating under the new rule.
- `tests/frame-decide-tests.c` (25 assertions) pins the V6 regression —
  `detection_degraded` + confident rect + valid geometry must flag the frame
  (chip) AND still map the mask — plus reason-precedence ordering,
  stale-drops-all-masks, INVALID-among-OK keeps the OK rect, off-capture
  silent skip.
- Artifacts all present, including the new `frame-decide-tests.exe`. The
  perf-instrumented DLL deployed for the owner's soak
  (`D:\software\obs\obs-studio\obs-plugins\64bit\streamsentry.dll`,
  2026-07-10 00:25) was corroborated as this exact revision: contains the
  run-3 reason string, byte-size equals the PERF_LOG=ON build.

### Gate history (three rounds; the FAIL texts are preserved verbatim in the guardian report)
`reports/M6.5-spec-guardian.md` keeps all three rounds;
`reports/GATE-CATCHES.md` indexes them (cases 3–5).
1. Round 1 — FAIL, five violations plus one question: V1 monitor-enumeration
   failure silently stopped toast masking (no log, no chip, heartbeat still
   fresh); V2 README still claimed the blackout; V3 two live log strings
   still claimed fail-closed; V4 six ARCHITECTURE.md lines contradicted the
   amended law; V5 the guardian's own checklist still mandated blackout; Q1
   asked which way cloak-query doubt should fall.
2. Round 2 — FAIL: the V1 fix (`detection_degraded`) had been folded into
   `unverified` ahead of the mapping block, so degraded frames dropped even
   confident blocklist/password masks (V6) — the exact under-masking the
   ruling guards against; the guardian noted the staged docs already
   promised the correct behavior ("doc matches the law; the code is the
   bug") and that no automated test covered the path.
3. Round 3 — PASS: the decision logic was extracted into the pure
   `frame-decide` module and the V6 case became a permanent unit test; all
   of V1–V5, V6, and Q1 verified resolved; no new violations.

After the round-3 PASS, three doc-only wording fixes explicitly listed as
non-blocking suggestions in the guardian's own round-3 report were applied
(ARCHITECTURE.md dependency paragraph and data-flow diagram now include
frame-decide; SPEC.md "protection-inactive" adjectives updated to
"protection-degraded"); no code changed after RUN-3.

### Not covered by automation (from the verifier)
- In-OBS visuals: chip appearance and clearing, and masks persisting on
  degraded frames — the frame decision is unit-tested against synthetic
  snapshots, but the `src/filter.c` glue and the pixels it draws are
  compile-verified only.
- The `detection_degraded` path end-to-end (a real zero-monitor condition →
  flag → chip while rendering continues) has no automated fixture; the
  cloak-query-failure skip and the degraded-transition log lines are
  verified by inspection and compilation only (the self-test cannot force a
  `DwmGetWindowAttribute` failure).
- Improvement over M6: the V6 failure mode and the failure-reason precedence
  ordering are now unit-covered (`frame-decide-tests`) instead of relying on
  review or manual testing.
- Ruling provenance: the verifier confirmed the staged law docs match the
  ruling record point-for-point, but no automation can verify the record
  faithfully captures the owner's decision — that attestation is the
  owner's, at acceptance (item 6 below).

### Manual acceptance — STATUS: PENDING (owner's pass; still current — the M7 section below ADDS the allowlist/panic items; run both)
Items 1–3 and 6 are new or changed by M6.5; items 4–5 and 7–8 carry forward.
Steps for the carried v0.1 items live in `reports/HUMAN_CHECKLIST.md`.
1. **Degraded-chip check** — REPLACES the v0.1 "kill watcher (fault
   injection) → full black ≤ 500 ms + status text" acceptance row (SPEC
   Part 1 matrix as amended by §2.7). Fault-inject watcher death/stall in
   OBS → the source KEEPS RENDERING unmodified; a small opaque top-left
   chip "StreamSentry: protection degraded - see log" appears within
   ≤ 500 ms; the OBS log gains one `PROTECTION DEGRADED: <reason>` line.
   On recovery the chip clears and `protection restored: full masking
   active again` is logged. Note: on sources smaller than 160×60 px a
   geometry floor skips only the on-output chip — the log line still fires
   (guardian §5; ARCHITECTURE.md).
2. **Masks persist while degraded** (the V6 property, on screen): with a
   blocklisted window plated, force the degraded state → the privacy plate
   must STAY up while the chip shows.
3. **30-minute streaming soak** (SPEC 2.1 acceptance, carried from M6
   item 1 with the grep targets RENAMED by M6.5 — the old strings no longer
   exist in the binary):
   - expect ZERO `PROTECTION DEGRADED: heartbeat stale` lines (replaces
     `FAIL-CLOSED engaged: heartbeat stale`);
   - the slow-tick early warning line is now `watcher tick took N ms (early
     warning; detection-stale threshold is 500 ms)` (was "fail-closed stale
     threshold");
   - `PERF watcher tick` line format unchanged — record the tick p99
     (target ≤ 50 ms) and the pid-cache hit rate in `reports/`.
   The deployed soak DLL was refreshed to the M6.5 revision on 2026-07-10
   00:25; a soak log captured before that came from an older binary.
4. **Flyout storm check** (SPEC 2.2) — unchanged from M6 item 2, including
   the build-26200.8655 taxonomy note.
5. **Blocklist regression** — unchanged from M6 item 3.
6. **Owner countersign of the ruling's Application addendum** (requested by
   the spec-guardian, rounds 2–3): confirm the three derivative decisions
   made by applying the ruling's principles — (a) cloak-query doubt now
   skips the window, (b) chip label generalized to "protection degraded -
   see log", (c) the guardian's own checklist amendment — plus the recorded
   Q1 residual: a genuinely visible window whose cloak query persistently
   fails is skipped per-window without a notice.
7. **DEFERRED — blocked, not forgotten** (carried from M6 item 4): real
   toast masked before content is readable, empirical calibration of the
   PROVISIONAL gate constants, and re-verification of the v0.1 toast
   signature on build 26200.8655 — still blocked while toast banners are
   system-suppressed on this machine; the M6 diagnostic note (split "banner
   never displayed" from "detection missed it") applies unchanged. New
   under §2.7: with monitor data unavailable, an unmasked toast is the
   DESIGNED outcome and must coincide with the degraded chip — silence
   would be a bug.
8. **Carried v0.1 items** (unchanged from M5/M6): end-to-end password-field
   privacy plate on a real focused field; coordinate landing accuracy on a
   second monitor / different DPI and on a scaled or cropped capture; the
   full 2-hour endurance soak; one glance that the filter entry reads
   **StreamSentry**.

---

## M7 — 2026-07-10 — Allowlist mode + panic hotkey (SPEC Part 2 items 2.3 + 2.4)

### What was built
- **Allowlist mode** (SPEC 2.3): a **Mode** dropdown (Blocklist / Allowlist)
  in the filter settings. Blocklist is the default and a pre-M7 settings blob
  resolves to blocklist — v0.1/M6 behavior is preserved on upgrade. In
  allowlist mode every visible, non-cloaked top-level window that does NOT
  match the approval list gets an opaque privacy plate; approved windows pass
  through. Matching semantics are identical to the blocklist (owner ruling
  a434b18: case-insensitive substring vs process image name OR window title).
  The allowlist is stored under its own settings key — a mode switch never
  reinterprets one list as the other, and the properties UI shows only the
  active mode's textbox. An empty allowlist approves NOTHING (no default
  fallback) and there are no implicit approvals: taskbar and wallpaper are
  masked until explicitly approved. Toast and password detection stay active
  in both modes — approving a process does NOT exempt its toasts.
- **Allowlist failure direction**: any unverified frame (no snapshot, stale
  heartbeat > 500 ms, mode-switch transient, unresolved capture geometry,
  mapping failure, detection degraded, filter-begin failure) draws ONE
  full-source opaque privacy plate instead of the source — the mode's own
  opted-into default (default-deny), the designed exception under CLAUDE.md
  rule 1. Blocklist mode keeps the M6.5 semantics everywhere (source renders
  + chip; confident masks kept).
- **Rect-budget overflow** (closes the guardian observation carried since
  M5): more detections than the 64-rect snapshot budget → the watcher
  publishes `mask_all`. Allowlist renders the full-source plate (the mode
  default, no chip); blocklist keeps every rect it did publish and shows a
  "detection overflow (some masks dropped)" chip + warning log — the cap is
  no longer a silent drop in either mode.
- **Panic hotkey** (SPEC 2.4): one OBS hotkey per filter instance,
  "StreamSentry: mask everything (panic)", registered via
  `obs_hotkey_register_source` (bound under OBS Settings → Hotkeys; no
  properties-UI element), toggle semantics. Engaged → the render draws a
  full-source opaque privacy plate INSTEAD of the target (the branch sits
  before `process_filter_begin`, so the target is never composed into the
  frame). The degraded chip stacks on top when the frame is simultaneously
  unverified. Engage/release are logged; the engage log gains the suffix
  "(filter currently disabled - takes effect when enabled)" when Enable is
  off (guardian Q3). Not persisted across sessions.
- **Guardian round-1 remediations** (all re-verified in round 2):
  - V1: a genuine `EnumWindows` failure no longer publishes a normal-looking
    tick — the tick is skipped (no snapshot, no heartbeat) and
    transition-logged, so the render side goes unverified within 500 ms via
    the existing stale trigger (allowlist → mask-all, blocklist → chip). The
    heartbeat again certifies only a completed pass.
  - V2: the `process_filter_begin`-failure branch is mode-aware — allowlist
    with masks pending draws the full-source plate instead of skipping the
    filter (default-deny no longer fails open on that path); blocklist keeps
    skip + chip.
  - Q1: allowlist + `detection_degraded` → mask-all (default-deny must not
    depend on which processes the user approved — an approved toast host
    would otherwise show a real toast during degradation). Blocklist +
    degraded keeps the M6.5 V6 guarantee (confident masks + chip), now
    pinned by its own unit case.
  - Q2: the cloak query is tri-state and mode-aware — blocklist skips a
    window whose DWMWA_CLOAKED query fails (no mask on doubt), allowlist
    masks it unless approved (no confidence-less pass-through hole in
    default-deny).
- Tests: `frame-decide-tests` grew 25 → 55 assertions (10 M7 decision cases,
  the allowlist-degraded flip, and a new blocklist-degraded companion);
  `watcher-selftest` gained two allowlist legs. Six new locale strings.
  `CMakeLists.txt` untouched (blob-identical to M6.5, verifier-pinned).

### Automated evidence
Sources: `reports/M7-verifier.md` (Verdict: VERIFIED — two full from-scratch
runs: run 1 on the pre-audit tree, then the guardian's round-1 findings drove
code changes and the RUN-2 ADDENDUM re-ran the entire sequence against the
final staged tree pinned by blob hash; RUN-2 is authoritative) and
`reports/M7-spec-guardian.md` (round-2 verdict: PASS), both 2026-07-10.
- Commands (RUN-2, verbatim from the report):
  ```
  rm -rf F:/obsplugin/build_x64
  cmake --preset windows-x64-local
  cmake --build --preset windows-x64-local
  cmake --preset windows-x64-local -DSTREAMSENTRY_PERF_LOG=ON
  cmake --build --preset windows-x64-local
  cmake --preset windows-x64-local -DSTREAMSENTRY_PERF_LOG=OFF
  cmake --build --preset windows-x64-local
  ctest -C RelWithDebInfo --output-on-failure          # cwd: F:/obsplugin/build_x64
  ./coord-map-tests.exe; ./plate-gen-tests.exe; ./toast-gate-tests.exe; ./frame-decide-tests.exe
  ./watcher-selftest.exe                               # cwd: build_x64/RelWithDebInfo
  ```
- Results (run 2, 2026-07-10 11:44–11:49): every configure/build exit 0 with
  zero warnings in every compile/link log (EN + localized zh-TW patterns;
  both `STREAMSENTRY_PERF_LOG` variants; tree left OFF — the shipping
  configuration). The only warnings anywhere are the two known pre-existing
  configure warnings from the vendored OBS sources in `.deps` (FindDetours
  version / virtualcam GUID) — out-of-tree, not from this diff.
- ctest 4/4 suites PASS; each executable also run directly, "all passed",
  exit 0 — 137 static CHECK assertions total (coord-map 35, plate-gen 24,
  toast-gate 23, frame-decide 55).
- `watcher-selftest.exe` exit 0 — all deterministic legs pass, including the
  two NEW allowlist legs: "allowlist mode masks unapproved windows (rects or
  mask_all)" and "blocklist mode restored after switching back". The toast
  leg is INCONCLUSIVE — expected: banners are still system-suppressed on
  this machine (`reports/M6-toast-probe.txt`); not a regression.
- Artifacts present in `build_x64/RelWithDebInfo/` (dll 74,752 B, final
  PERF_LOG=OFF build). String check on the fresh dll: the run-2 strings
  present ("EnumWindows FAILED", "takes effect when enabled", "PANIC engaged
  by hotkey"), `PERF watcher tick` absent — the tree ships OFF.
- Deployment: the owner-pass DLL at
  `D:\software\obs\obs-studio\obs-plugins\64bit\streamsentry.dll` was
  refreshed 2026-07-10 11:43 and corroborated as the M7-r2 PERF_LOG=ON build
  (contains both r2-distinguishing strings AND the PERF string; byte size
  equals the run's ON build — with the verifier's recorded caveat that
  run-1's ON build had the same byte size, so the r2 identification rests on
  the string evidence).

### Gate history (two rounds; the round-1 FAIL is preserved verbatim in the guardian report)
`reports/M7-spec-guardian.md` keeps both rounds; `reports/GATE-CATCHES.md`
case 6 indexes the catch.
1. Round 1 — FAIL, three violations plus three questions: **V1** a genuine
   `EnumWindows` failure published a normal-looking snapshot (fresh
   heartbeat, missing rects) — in allowlist mode a silent pass-through of
   unapproved windows, the exact direction SPEC 2.3 forbids; **V2** the
   filter-chain-bypass path showed unapproved windows chip-only in allowlist
   instead of the mode's mask-all default (the staged ARCHITECTURE.md
   contradicted the staged code); **V3** no TESTING.md note in the change
   set (CLAUDE.md workflow rule, binding at landing per M2–M6.5 precedent).
   **Q1** the degraded-only exception's justification failed for approved
   toast hosts; **Q2** cloak-doubt skip = a confidence-less pass-through
   hole in allowlist; **Q3** panic vs the Enable checkbox.
2. Round 2 — PASS: V1 and V2 fixed in code and re-verified line-by-line
   against the staged blobs (see remediations above); Q1–Q3 resolved with
   recorded, principle-derived directions (Q1/Q2 in
   `reports/RULING-2026-07-09-fail-open.md`, Application addendum 2 items
   5–6; Q3 via the self-documenting log suffix). V3 was ACCEPTED as
   satisfied-by-process with BINDING CONDITIONS: (a) the milestone commit
   MUST include this TESTING.md M7 note — **this section fulfills that
   condition**; (b) landing without it would void the PASS. Two further
   conditions: the R1/R2 doc-only ARCHITECTURE.md corrections before commit
   assembly (verified applied in the working tree — the test-infrastructure
   frame-decide row and trigger-table row 8 now carry the corrected
   wording), and re-staging the guardian report so the commit carries both
   rounds verbatim.

### Not covered by automation (from the verifier)
- In-OBS panic behavior end-to-end: a real hotkey binding, next-frame plate
  visibility, fresh-session reset, and the panic-while-disabled log suffix
  all need a live OBS runtime.
- Allowlist rendered pixels: approved-app-visible / others-plated,
  taskbar/desktop plated until approved, the one-tick mode-switch transient,
  more than 64 unapproved windows → single full-source plate, allowlist +
  watcher killed → mask-all within 500 ms. The decision layer is unit-tested
  against synthetic snapshots and the self-test proves live rect/mask_all
  publication, but the pixels are owner-manual.
- Properties-UI show/hide and the mode round-trip acceptance row (obs_data
  settings need the OBS runtime; the separate keys were verified by
  inspection).
- The EnumWindows-failure skip-publish path and the CLOAK_UNKNOWN handling:
  inspection + compilation only (the self-test cannot force those APIs to
  fail); the resulting direction is covered generically via the stale-path
  unit tests.
- The mode-aware begin-failure branch and all render-path pixel drawing
  (full-source plate, chip, per-rect plates, solid-fill fallback):
  compile-verified only — only the pure modules are unit-tested.
- Multi-instance last-writer-wins mode/list semantics: documented in
  watcher.h, not exercised by automation.

### Manual acceptance — STATUS: PENDING (owner's M7 pass; ADDS to the M6.5 checklist above — both lists are current)
Items 1–5 are the SPEC v0.2 acceptance rows for 2.3/2.4; item 6 is blocked;
items 7–8 are the residual/countersign records required by the guardian's
round-2 PASS.
1. **Allowlist basic (default-deny)**: set Mode = Allowlist and approve
   exactly one app (one line, e.g. its process name) → that app is visible
   and EVERY other window gets a per-window privacy plate — including the
   taskbar and desktop/wallpaper, until explicitly approved (approving
   `explorer.exe` is the expected way to unmask the shell).
2. **Allowlist overflow**: with more unapproved windows than the 64-rect
   budget, expect ONE full-source privacy plate (the mode's default — NOT
   black, no chip).
3. **Mode-switch round-trip**: fill both lists; switch blocklist → allowlist
   → blocklist. Both textbox contents must survive untouched, and the
   properties UI must show only the active mode's list. A one-tick "mode
   transition pending" transient (chip; mask-all if the filter is in
   allowlist mode) is expected at the switch itself.
4. **Panic hotkey**: bind "StreamSentry: mask everything (panic)" in OBS
   Settings → Hotkeys. Press → full-source opaque plate on the very next
   frame INSTEAD of the source; press again → normal rendering. Check in
   BOTH modes. Engage during fault injection (watcher killed) → the plate
   STAYS and the degraded chip stacks on top. Restart OBS with panic
   engaged → the fresh session starts released (not persisted).
5. **Allowlist + watcher killed** (fault injection): in allowlist mode, kill
   the watcher → full-source mask-all plate within ≤ 500 ms plus the
   degraded chip — NOT black, NOT pass-through. (Blocklist mode keeps the
   M6.5 behavior: source renders + chip.)
6. **BLOCKED — toast exemption from approval** (SPEC 2.3): in allowlist mode
   with the toast host approved (e.g. `explorer.exe`), a real toast banner
   must STILL get the notification card — approval must not exempt toasts.
   Blocked with the M6/M6.5 deferred real-toast items while banners are
   system-suppressed on this machine; the M6 diagnostic note (split "banner
   never displayed" from "detection missed it") applies unchanged.
7. **Panic while disabled (Q3 residual)**: with the filter's Enable checkbox
   OFF, toggling panic masks nothing until re-enabled; the engage log line
   says so ("(filter currently disabled - takes effect when enabled)") and
   panic takes effect on the next rendered frame once Enable is back on.
8. **Owner countersign — M7 derivative rulings** (requested by the guardian;
   same mechanism as the M6.5 item 6 countersign — sign both together):
   - Q1: allowlist + detection-degraded → mask-all
     (`reports/RULING-2026-07-09-fail-open.md`, Application addendum 2
     item 5);
   - Q2: cloak-query failure is mode-aware — blocklist skips on doubt,
     allowlist masks unless approved — including the accepted guardian R3
     residual: in allowlist mode a genuinely cloaked toast-signature window
     whose cloak query fails may receive a toast card (mask-more, the mode's
     bias) (addendum 2 item 6);
   - Q3: the explicit Enable off-switch outranks panic (nothing is masked
     while disabled); direction recorded in the guardian report, round 2.
9. **Carried**: everything in the M6.5 checklist above (items 1–8) remains
   open and current — M7 adds items rather than replacing any.
