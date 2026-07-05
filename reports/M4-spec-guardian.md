# M4 Spec-Guardian Audit — StreamSentry v0.1 (ship-ready)

- Date: 2026-07-05
- Auditor: Spec Guardian (docs-honesty + scope pass; code was audited M1/M2/M3)
- Repo: F:\obsplugin (branch master, HEAD = M3 commit 3a6bf59)
- Change set audited (working tree vs HEAD):
  - README.md — full rewrite (M0-skeleton status -> ship-ready), 133 insert / 45 delete
  - reports/FINAL.md — new
  - reports/M4-verifier.md — new
  - release/ — gitignored build artifact (.gitignore:2:/* confirmed), IGNORED per task
- Method: every README/FINAL claim cross-checked against reports/M1-verifier.md,
  M2-verifier.md, M3-verifier.md, M2-DECISIONS.md, M3-acceptance-matrix.md,
  HUMAN_CHECKLIST.md, plus buildspec.json and LICENSE ground truth.
- Governing law: CLAUDE.md iron rules + SPEC.md v0.1 locked scope. No source code was
  modified by M4 (git status shows only README.md modified + two new reports/*.md).

---

## 1. README truthfulness vs actual code behavior

### 1a. Nothing verified-only-by-human is presented as proven
- Header status banner (README:8-10): "built and machine-tested; several real-device
  checks are still a manual pass." Honest framing up front. PASS.
- Toast masking: README does NOT claim it is verified end-to-end. Described as a
  capability "always on while the filter is enabled" (README:85-86); over-masking
  disclosed in Limitations (README:98-101); the INCONCLUSIVE-under-DND result is carried
  honestly in FINAL.md:53-55 and M4-verifier.md:62. Matches acceptance-matrix row 1
  (MECHANISM + HUMAN). PASS.
- Live password-field masking: README:85-86 "always on", not claimed machine-verified.
  Chromium on-demand a11y (README:107-108) and custom-drawn-UI gaps (README:105-106)
  disclosed. Matches acceptance-matrix row 2 (MECHANISM + HUMAN). PASS.
- Multi-monitor / DPI / scaled accuracy: "Building from source" (README:142-143) scopes
  tests as "Pure-logic modules (coordinate mapping, plate generation) have unit tests" —
  NOT claimed as live end-to-end proof. Limitations (README:92-97) states identical-res
  dual monitors, scaled/cropped, and Window Capture FAIL CLOSED. PASS.

### 1b. v0.1 limitations honestly disclosed
- Display-Capture-only / fails-closed on window+scaled capture + dual identical-res
  monitors — README:92-97. Matches M2-DECISIONS.md + geom-resolve audit. PASS.
- Toast over-masking (shared XAML popup class) — README:98-101. Matches M2-DECISIONS.md.
  PASS.
- 1-2 frame exposure gap — README:110-111. Matches SPEC.md:41. PASS.
- IME / RDP / VM / fullscreen-game / Chromium — README:107-113. Matches SPEC.md:38-43.
  PASS.
- Content-level secret detection permanently out of scope — README:114-117. Matches
  SPEC.md:35. PASS.
- English-only — README:118. Matches SPEC.md:35. PASS.
- UAC secure-desktop note — README:102-104 correctly: UAC on secure desktop not
  capturable by OBS; normal-desktop credential broker is blocklisted. Matches
  acceptance-matrix row 3. PASS.

### 1c. Fail-closed / deterministic / opaque described correctly
- Fail-closed non-disableable: README:41-42 "cannot be turned off; there is no setting
  for it"; Usage:82-83 "deliberately no option to disable the fail-closed blackout."
  Matches iron rule 1 + M3-verifier Check 4. PASS.
- Starts black until proven alive: README:41. Matches M2-obs-integration RUN A. PASS.
- Deterministic / no AI: README:30-34 "no machine learning anywhere in this plugin."
  Matches iron rule 2. PASS.
- Opaque never blur: README:44-49. Matches iron rule 3 (section 2). PASS.

### 1d. Performance numbers
README:120-122 (watcher ~0.55% core, render ~0.03 us/frame, 30-min soak stable, 2-hour
run manual) match M3-perf.md exactly (M3-verifier Check 5); 2-hour correctly labeled
still-manual. PASS.

Section 1 verdict: PASS. No overclaim; INCONCLUSIVE/manual items disclosed as such.

---

## 2. Iron rule 3 — no blur/pixelate offered or implied
- README:44-49 "solid, opaque plates", "never blurs or pixelates", correct rationale
  (blur/mosaic reversible, archived clips attackable offline). Raw black reserved for
  fail-closed.
- README:129 (Roadmap "Never planned") lists "blur/mosaic options" as never planned — no
  blur/pixelate option offered, implied, or teased. No such option in Usage (README:75-86).
  Matches src grep (M3-verifier Check 6: zero blur/mosaic in src/).

Section 2 verdict: PASS.

---

## 3. Iron rule 5 — scope lock
- README Roadmap (README:124-130) places allowlist mode, Focus Assist/DND auto, window-
  capture geometry, macOS, per-app policies under "Deliberately deferred"; blur/mosaic,
  AI, content secret scanning, tray icon, auto-update under "Never planned." Matches
  SPEC.md:34-35.
- None advertised as a shipping feature anywhere. Only configurable surface = Enable +
  Blocklist textbox (README:75-86) = SPEC.md:29-33. Localization listed out (README:118).
- FINAL.md:70-92 "To publish": every step human-side, nothing auto-pushed. Step 2
  git push shown as an instruction (M4 pushed nothing — 3 commits ahead, unpushed);
  step 3 tag TRIGGERS CI producing a DRAFT; step 4 "will not go public until you press
  publish"; FINAL.md:5 "Nothing has been pushed to the internet." PASS.

Section 3 verdict: PASS.

---

## 4. Iron rule 6 — license consistency
- README:147 "GPL-2.0-or-later — see LICENSE. All original code."
- LICENSE header "GNU GENERAL PUBLIC LICENSE / Version 2, June 1991", 338 lines (full
  GPLv2). SPDX "GPL-2.0-or-later" label over a GPLv2 file is the CLAUDE.md rule 6
  convention. No "adapted from another project" claim. PASS.

Section 4 verdict: PASS.

---

## 5. buildspec.json version vs README stated version/status
- buildspec.json:39-41 -> name "streamsentry", displayName "StreamSentry", version
  "0.1.0". (version strings at lines 4/13/22 are nested dependency versions — OBS
  31.1.1, obs-deps — not the plugin version.)
- README download streamsentry-0.1.0-windows-x64.zip (README:58) and status "v0.1,
  pre-release" (README:8) consistent with buildspec 0.1.0. Corroborated by M4-verifier
  Check 4/6. PASS.

Section 5 verdict: PASS.

---

## 6. FINAL.md accuracy (passed vs pending; real evidence; open decision; not "released")
- Machine-verified list (FINAL.md:22-46) matches evidence: warnings-as-error build +
  no-Qt/no-third-party (M4-verifier Check 1/3), unit tests coord-map 11 cases + plate
  opacity alpha=255 (M1-verifier Check 2, M4-verifier Check 2), fail-closed <=500ms
  (M2-obs-integration, watcher-selftest, row 7), blocklist appear/clear + heartbeat
  freeze (watcher-selftest), perf ~0.55% / 0.03us + 30-min soak (M3-perf.md), pass-
  through (row 10). All cited files exist in reports/. PASS.
- Needs-a-human list (FINAL.md:48-59): visual opacity, real toast with DND OFF (labels
  automated toast INCONCLUSIVE), live password-field + real blocklist app, real 2nd-
  monitor/DPI + scaled source, full 2-hour endurance. Matches HUMAN_CHECKLIST.md +
  acceptance matrix; INCONCLUSIVE stated honestly, not hidden. PASS.
- Open decision (FINAL.md:61-68): blocklist match semantics = process-name OR title-
  substring; CLAUDE.md literal "process AND class" applied to toasts only; described as
  "your call"/awaiting ruling. Matches M2-DECISIONS.md + HUMAN_CHECKLIST item 10.
  Correctly surfaced, not buried. PASS.
- "Released" wording: FINAL.md:1 "mission complete through M4" + title "FINAL report" =
  ship-READY; FINAL.md:5 + FINAL.md:70 make explicit nothing is published; milestone row
  M4 (FINAL.md:17) = "release-zip dry-run", not a published release. No "released to the
  public" claim. PASS.
- Non-blocking note (NOT a violation): FINAL.md milestone-table commit short-hashes are
  the scribe's recorded values; current audited HEAD is 3a6bf59 (M3) with M4 as working-
  tree. Historical refs in a capstone doc; affect no guarantee.

Section 6 verdict: PASS.

---

## 7. No NEW scope/feature introduced in docs beyond the code
- README Usage settings (README:75-86) = Enable checkbox + Blocklist multi-line textbox +
  no-disable-fail-closed = the shipping surface (M3-verifier Check 3: exactly two
  properties, bool "enabled" + OBS_TEXT_MULTILINE "blocklist"). No invented setting.
- Default blocklist "password managers and credential dialogs" (README:80-81) matches
  HUMAN_CHECKLIST item 1 defaults.
- "Clear the box to fall back to the built-in defaults" (README:80-81) matches
  get_defaults (M3-verifier Check 3b).
- Toast + password "always on while enabled" (README:85-86) = existing behavior, not a
  new promise. No README-only capability/API/setting the plugin lacks. PASS.

Section 7 verdict: PASS.

---

## Iron-rule roll-up
- Rule 1 (fail-closed, non-disableable): PASS — README:41-42, 82-83.
- Rule 2 (deterministic, no AI): PASS — README:30-34.
- Rule 3 (opaque, never blur/pixelate): PASS — README:44-49, 129.
- Rule 4 (no third-party deps): PASS — README introduces no dep; FINAL.md no-Qt claim
  matches M4-verifier Check 3 dependents.
- Rule 5 (v0.1 scope lock): PASS — Roadmap correct; publish left to human.
- Rule 6 (GPL-2.0-or-later, original): PASS — README:147 over GPLv2 LICENSE.
- CLAUDE.md vs SPEC.md conflict: the blocklist AND-vs-OR tension exists in the SOURCE
  docs; M4 (docs-only) neither introduces nor resolves it and correctly carries it to the
  human (FINAL.md:61-68, HUMAN_CHECKLIST item 10). Not introduced by this change set, so
  it does not FAIL M4.

## QUESTIONS for the human (non-blocking)
Q1. Blocklist match semantics remain process-name OR title-substring (SPEC wording +
    iron rule 1 never-under-mask); CLAUDE.md literal "process AND class" applied to toasts
    only. README:79-80 describes the OR behavior. Confirm OR-semantics is your final
    ruling before publish (strict AND is a one-line, less-safe change).
Q2. FINAL.md publish step 2 lists remote git@github.com:darker901-chen/StreamSentry.git.
    Confirm that destination before you push; the guardian only verifies M4 pushed nothing
    itself (3 commits remain local/unpushed).

---

RESULT: PASS
