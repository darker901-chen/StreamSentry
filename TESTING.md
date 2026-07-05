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
