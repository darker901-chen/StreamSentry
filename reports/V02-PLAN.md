# v0.2 PLAN — approved by owner 2026-07-05 ("全部作")

Owner approved all five items after hands-on field testing of v0.1.
Same gate workflow per milestone: implement → verifier (VERIFIED) →
spec-guardian (PASS) → scribe → commit `M<n>: ...`. Owner pushes to
GitHub themselves — the agent NEVER pushes.

## Field-test findings that drive v0.2 (owner's live session, 2026-07-05)

1. **Occasional fail-closed blips under streaming load.** OBS log shows
   repeated `FAIL-CLOSED engaged: heartbeat stale (age 501-505 ms)`
   clearing ~100-150ms later, roughly once a minute while SRT-streaming.
   Suspect: per-tick OpenProcess/QueryFullProcessImageNameW for every
   visible window; plan = cache PID→name, measure tick p99, consider
   watcher thread priority. (M6)
2. **Toast detection over-matches.** explorer.exe Xaml_WindowedPopupClass
   also matches many shell flyouts → 9-27 plates flapping, noisy screen.
   Plan: narrow by toast geometry/position (top-right spawn region, size
   bounds) while staying over-mask-safe. (M6)
3. **Blocklist UX is developer-grade.** Users can't know names like
   `credentialuibroker`. Plan: "list open windows" picker with checkboxes
   in properties. (M8)
4. **Blocklist logic can't protect against the unanticipated** (owner hit
   this on a payment site — plain text field, no OS password marking, not
   in any list). Structural fix = **allowlist mode**: default-mask
   everything, only approved windows show. (M7, the headline)
5. Chromium password fields undetected (a11y tree off by default) —
   documented v0.1 limitation; investigate per-browser in v0.2 if time.

Validated in the field: SRT streaming works with the filter active
(OBS as listener `srt://192.168.68.52:8888?mode=listener`, player
connects to `srt://192.168.68.52:8888`); blocklist masking works
(log: up to 27 simultaneous plates); mid-plate rendering solid.

## Milestones

| # | Scope |
|---|---|
| **M5 docs+governance** | ARCHITECTURE.md (as-built v0.1: module map watcher/shared-state/coord-map/plate-gen/geom-resolve/filter, thread model, fail-closed state machine, data flow); SPEC.md v0.2 section (allowlist, panic hotkey, picker, toast narrowing, perf fix specs); CLAUDE.md scope lock v0.1 → v0.2 (all other iron rules unchanged) |
| **M6 hardening** | Fix heartbeat-stall blips (PID-name cache + measurements); narrow toast matching by geometry; re-run perf + soak + selftest |
| **M7 allowlist + panic key** | Mode switch blocklist/allowlist; allowlist = default-mask-all, approved windows pass; OBS hotkey "mask everything now" toggle; fail-closed semantics under allowlist (default-masked is already safe) |
| **M8 picker UI + ship** | "List open windows" button + checkbox selection in properties; docs/README/package refresh; v0.2 FINAL report |

## Standing owner rulings (do not re-ask)

- Blocklist matching = process-name OR title-substring. FINAL (commit a434b18).
- NO Co-Authored-By trailer on any commit, ever.
- Agent never pushes; owner pushes manually.
- Iron rules relaxable only by the owner, explicitly.
- Owner wants plain-language (白話) explanations and hands-on verification;
  do not discourage them from testing the real user experience.

## Environment notes for the next session

- Work in **F:\obsplugin** (session-protected primary; memory lives under
  the F--obsplugin project). **F:\StreamSentry** is the robocopy mirror —
  sync after each milestone: `robocopy F:\obsplugin F:\StreamSentry /MIR
  /XD build_x64 .deps release`.
- Local build: preset `windows-x64-local` (untracked CMakeUserPresets.json,
  SDK 10.0.26100). Deploy = copy build_x64\RelWithDebInfo\streamsentry.dll
  (+pdb) to D:\software\obs\obs-studio\obs-plugins\64bit\.
- OBS 32.1.2 at D:\software\obs\obs-studio; user's own scene collection is
  無標題; test collections StreamSentryM1/M2/M3 exist in
  %APPDATA%\obs-studio\basic\scenes (M1/M2 carry stale debug_* setting keys
  from removed scaffolding — harmless, may clean up in M6).
- Perf baseline (M3): watcher ~0.55% of one core, render decision
  ~0.03 µs/frame, 30-min soak flat ~321 MB.
- v0.1 history: M0..M4 + ruling = HEAD a434b18, all gates green, NOT yet
  pushed (owner will push).
