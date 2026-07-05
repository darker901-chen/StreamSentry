# StreamSentry v0.1 — FINAL report

Autonomous v0.1 mission complete through M4. This is the single-page
capstone: what is machine-verified, what still needs a human, and exactly
how to publish. Nothing has been pushed to the internet — publishing is a
human decision.

## Milestones (each passed verifier VERIFIED + spec-guardian PASS, then scribe)

| Milestone | Commit | What |
|-----------|--------|------|
| M0 | `11bd06a` / `b4393b0` | Skeleton pass-through filter; loads in OBS. |
| (rename) | `4366ad2` / `38f35d3` | obsplugin → StreamSentry. |
| M1 | `cb5b2b0` | Render path: opaque plates + fail-closed + pure coord-map, ctest. |
| M2 | `127781c` | Real detection: COM MTA watcher, Win32 enum, UIA password fields, geometry. |
| M3 | `3a6bf59` | Settings UI (blocklist textbox), perf, 30-min soak, acceptance matrix. |
| M4 | _this commit_ | README, release-zip dry-run, this report. |

Per-milestone evidence lives in `reports/M{n}-verifier.md` and
`reports/M{n}-spec-guardian.md`.

## What is machine-verified (with evidence)

- **Builds clean** with warnings-as-errors (CI-parity); `streamsentry.dll`
  imports libobs + Windows COM/DWM + the C++ runtime only — **no Qt, no
  third-party library** (`reports/M4-verifier.md`, M2/M3 verifier).
- **Unit tests** (`ctest`): coordinate mapping (11 cases: multi-monitor,
  negative origins, mixed DPI, scaled/cropped, padding clamped) and plate
  **opacity** (alpha=255 pixel-asserted; corner inset 5 < pad 12).
- **Fail-closed**, end to end: stale heartbeat / dead watcher / unresolved
  geometry / mapping failure → full black within ≤500 ms + status banner;
  starts black until the first heartbeat proves detection alive; the
  500 ms threshold is a compile-time constant and there is **no setting to
  disable it** (`reports/M2-obs-integration.txt`, `M2-watcher-selftest-output.txt`,
  acceptance row 7).
- **Real detection**: the watcher self-test shows a blocklisted window
  (notepad) masked on open and cleared on close, and the heartbeat freezing
  under fault injection; the toast signature was determined empirically on
  this machine (explorer.exe + `Xaml_WindowedPopupClass`) and recorded in
  code.
- **Opaque, deterministic, no AI, GPLv2** — audited every milestone by
  spec-guardian, zero violations.
- **Performance**: watcher ~0.55% of one core (target <1%), added render
  cost ~0.03 µs/frame; 30-min soak working-set stable, no leak
  (`reports/M3-perf.md`).
- **Pass-through** when nothing sensitive is present (acceptance row 10).

## What still needs a human (see `reports/HUMAN_CHECKLIST.md`)

1. On-screen visual confirmation of the plate/card look and full opacity.
2. Real **toast** masking with **Do Not Disturb OFF** (this machine's DND
   made the automated toast test INCONCLUSIVE — the watcher detects the
   window, but no banner renders under DND).
3. Live **password-field** masking in a browser; real **blocklist app**
   (1Password/KeePass) if installed.
4. Coordinate accuracy on a real **second monitor / different DPI** and a
   **scaled/cropped** source (v0.1 fails closed on scaled/window capture by
   design).
5. The full **2-hour** endurance run (30 min was done automatically).

## One decision awaiting your ruling

**Blocklist match semantics** (`reports/M2-DECISIONS.md`): implemented as
process-name **OR** title-substring (SPEC Detection §2 + iron rule 1
"never under-mask"). CLAUDE.md's literal "process AND class" was applied to
toasts only. Both gate reviewers ruled this a defensible reconciliation,
not a violation, but it is your call. Reversing it to strict AND is a
one-line change (and less safe).

## To publish (all human-side — nothing pushed)

1. **Rule on the blocklist-semantics question** above (or accept the
   default). Run the `HUMAN_CHECKLIST.md` pass.
2. **Push to GitHub** (repo `git@github.com:darker901-chen/StreamSentry.git`
   already exists, currently at M0-era; this local branch is ahead through
   M4):
   ```
   git push origin master
   ```
3. **Tag to trigger the release CI** (template workflow: tag → build →
   draft GitHub release with the zip):
   ```
   git tag 0.1.0 && git push origin 0.1.0
   ```
   The CI produces `streamsentry-0.1.0-windows-x64.zip` — the same layout
   verified locally in `release/` (dry-run):
   ```
   streamsentry/bin/64bit/streamsentry.dll (+ .pdb)
   streamsentry/data/locale/en-US.ini
   ```
4. **Review and publish** the draft release GitHub creates (it is created
   as a draft — it will not go public until you press publish).

## Not in v0.1 (roadmap, enforced by CLAUDE.md rule 5)

Allowlist mode (v0.2 headline), Focus Assist auto-DND (v0.2),
window-capture geometry, macOS, per-app policy, blur/mosaic, AI detection,
content secret scanning, tray icon, auto-update, localization.
