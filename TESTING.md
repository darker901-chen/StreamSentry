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
