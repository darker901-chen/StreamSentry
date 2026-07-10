# V02-FINAL.md — StreamSentry v0.2 final report (2026-07-10)

v0.2 delivers everything the owner approved on 2026-07-05
(reports/V02-PLAN.md) plus one mid-cycle constitutional repositioning
the owner ruled on 2026-07-09. Four implementation milestones, every
one gated by verifier (VERIFIED) + spec-guardian (PASS) + scribe;
per-milestone evidence under reports/M*-*.md, gate interceptions
indexed in reports/GATE-CATCHES.md (6 cases).

## What shipped

| Milestone | Content | Commit |
|---|---|---|
| M5 | ARCHITECTURE.md (as-built record), SPEC.md Part 2, scope lock v0.2 | a62be5a |
| M6 | PID→name cache (heartbeat-stall fix), tick p99 instrumentation + 250ms early warning, toast geometry gate (PROVISIONAL constants — banners system-suppressed on the dev machine, reports/M6-toast-probe.txt) | 409964e |
| M6.5 | **Fail-open repositioning (owner ruling)**: blackout removed; unverified protection renders normally + opaque "protection degraded" chip + logs; confident masks always kept (solid-fill fallback); pure frame-decide module + regression suite | 37d8145 |
| M7 | **Allowlist mode** (default-deny, separate storage, mode-aware failure directions, mask-all fallback, overflow handling both modes) + **panic hotkey** (full-source plate instead of the target, not persisted) | 706cda1 |
| M8 | Window picker (enumerate → pick → append to the active list, deduped), README/version/package refresh to 0.2.0, this report | (this commit) |

Version: buildspec.json 0.1.0 → 0.2.0. Release zip dry-run:
release\streamsentry-0.2.0-windows-x64.zip (bin/64bit + data layout,
matches README; release/ git-ignored).

## The constitution change (read this first if you're new)

Owner ruling 2026-07-09 (reports/RULING-2026-07-09-fail-open.md + two
application addenda): the product is a privacy **assist** — wrong
masking is worse than under-masking; the output is never disrupted;
masks are drawn only on confidence; failures are never silent (chip +
log). The v0.1 fail-closed blackout is gone. Allowlist mode is the
deliberate exception: it fails toward its own default (mask-all),
because default-deny is what that user opted into. CLAUDE.md iron
rules 1+3 and SPEC §2.7 carry the amended law; the spec-guardian
audits against it (its own checklist was amended under the ruling —
GATE-CATCHES case 3).

## Test / verification state

- Automated: 4 ctest suites (coord-map, plate-gen, toast-gate,
  frame-decide — 55 asserts pinning every failure direction incl. the
  V6 and Q1 regressions), watcher-selftest (live desktop integration:
  blocklist, allowlist round-trip, picker enumeration, kill→stale).
  All green at every milestone's final gate.
- Machine state: perf-instrumented M8 DLL deployed to the owner's OBS
  for acceptance; release zip is built PERF_LOG=OFF.

## Open items at ship (owner-manual, tracked in TESTING.md)

1. 30-min soak (Record or stream): zero "PROTECTION DEGRADED" lines,
   tick p99 ≤ 50ms from the PERF log, pid-cache hit rate.
2. Allowlist visuals, mode round-trip, panic hotkey behavior (incl.
   fault injection: mask-all plate ≤500ms, chip stacking).
3. Owner countersigns of the ruling addenda (M6.5 items + M7 Q1/Q2/Q3
   + R3 mask-more bias).
4. BLOCKED until toast banners display again on the dev machine
   (system-suppressed since ~2026-07-08, diagnosis in
   reports/M6-toast-probe.txt): real-toast masking row, toast-gate
   constant calibration, signature re-verification on 26200.8655,
   allowlist toast-exemption row.
5. Chromium password-field investigation (SPEC 2.6) — untouched this
   cycle, remains a documented limitation (non-gating by spec).

## Known-good baseline for v0.3 planning

Top backlog candidate (owner pain, 2026-07-09 discussion):
window-capture geometry support — masks on Window Capture sources
instead of the degraded chip, including ignore-detections-that-cannot-
reach-the-capture semantics. Also parked: same-resolution dual-monitor
disambiguation via display device paths.
