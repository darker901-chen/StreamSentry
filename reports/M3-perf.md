# M3 — performance & endurance measurements

Build: `windows-x64-local` with `-DSTREAMSENTRY_PERF_LOG=ON` (instrument-
ation compiled out of the shipping build). Machine: AMD Ryzen 9 7900X,
Windows 11 build 26200. Source: 1920×1080 color source + StreamSentry
filter, default blocklist, no sensitive windows (steady-state
pass-through).

## Watcher thread CPU (SPEC target: < 1% of one core)

Logged every 200 ticks (~30 s) as `PERF watcher tick`. Representative
samples:

| avg ms/tick | % of one core (150 ms cadence) |
|-------------|-------------------------------|
| 0.826 | 0.55% |
| 0.803 | 0.54% |
| 0.819 | 0.55% |
| 0.816 | 0.54% |

**Result: ~0.55% of one core — under the < 1% target.** The tick cost is
dominated by the full top-level `EnumWindows` pass with per-window process
image-name lookup; it is steady, not growing.

## Render callback decision cost (SPEC: no visible frame drops)

The per-frame CPU work StreamSentry adds (shared-state read + heartbeat
check + geometry resolve + coordinate mapping; GPU draw submission
excluded as that is libobs). Logged every 600 frames as
`PERF render decision`:

- **avg 0.03 µs/frame** (≈30 ns), stable across the run.

At 60 fps a frame budget is 16 667 µs; 0.03 µs is ~0.0002% of it. In the
steady state (no rects) this is the whole added cost; when plates are
drawn there is additional GPU submission, but the decision path itself is
negligible. **No measurable frame-drop risk.**

## Endurance / memory-leak soak

30-minute run (SPEC's full target is 2 h — see HUMAN_CHECKLIST), OBS
working set + private bytes + handles + threads sampled every 30 s while
firing a toast every ~2 min. Raw data: `reports/M3-soak-samples.csv`.

**Result (60 samples over 30 min, 18:01–18:31):** stable, no leak
indicators.
- Working set: 345 MB at startup, trimmed to ~320–321 MB by ~160 s and
  **flat for the remaining 27 min** (max 321.1, no upward trend).
- Private bytes: **356.5 MB flat** the entire run.
- Handles: oscillate 6351–6382 in a narrow band, **no monotonic growth**.
- Threads: 234–242, stable (the single watcher thread is created once).
- Watcher tick cost held 0.51–0.55% of one core across the whole run (no
  drift upward); render decision stayed 0.03 µs/frame across 150 samples
  (~90 000 frames).

Steady-state correctness during the soak (OBS log `2026-07-05 18-01-27`):
exactly one FAIL-CLOSED at startup (the black-until-first-heartbeat
state), cleared 139 ms later; **no fail-closed and no plates for the rest
of the 30 min** — i.e. clean pass-through with no sensitive windows
present. This also satisfies acceptance-matrix row 10 (empty blocklist /
no password field → fully transparent pass-through).

Caveat: SPEC's endurance target is a **2-hour** run; this was 30 min per
the mission. A 2-hour confirmation is on the human checklist. The 30-min
trend shows no leak.
