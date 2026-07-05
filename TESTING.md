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
