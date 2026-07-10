# SOAK-EVIDENCE.md — owner-session performance evidence (v0.2 acceptance)

Log lines extracted by the agent from the owner's OBS logs; raw logs
stay in %APPDATA%\obs-studio\logs (not committed).

## Session 1 — 2026-07-10 16:34–16:45 (INTERIM, not the formal soak)

Build: M7-r2 perf-instrumented deploy (PERF_LOG=ON; version string
still 0.1.0 — the buildspec bump landed in M8 after this session).
Conditions: ~10 min interactive desktop use, NO recording/streaming
(no encode load). Log: 2026-07-10 16-34-30.txt.

Acceptance-relevant numbers (SPEC 2.1):

- Tick p99 across all seventeen 200-tick windows: **1.08–1.56 ms**
  (target ≤ 50 ms; v0.1 field failure mode was 501–505 ms). Avg
  0.79–0.85 ms ≈ 0.55% of one core — matches the v0.1 baseline while
  doing strictly more work (geometry gate + monitor enumeration).
- pid-cache hit rate: 99.9% on the cold first window (8963 lookups),
  100.0% steady-state (3600 lookups/window), evictions ≤ 1.
- Stale-heartbeat degradations during operation: **zero**.
- One early-warning spike: 16:43:29 tick took 362 ms (many windows
  opened at once; below the 500 ms threshold, no degradation — the
  M6 warning mechanism worked as designed).
- Render decision: 0.05–6.24 µs/frame (upper value while 9 plates
  were being mapped).

Functional evidence in the same log:

- Blocklist masking: up to 9 simultaneous plates.
- Allowlist mode switch (16:44:36): "mask-all engaged (allowlist
  default)" during the one-tick transient with the chip, released
  ~40 ms later into 9 per-window plates — exactly the specified
  transition behavior (SPEC 2.3 + 2.7).
- Filter remove/re-add: clean watcher stop/start; on re-add the
  41-second-old snapshot correctly showed "PROTECTION DEGRADED:
  heartbeat stale" + chip for ~40 ms until the fresh watcher
  published ("protection restored").
- Cosmetic: OBS zh-TW UI logs "Failed to load 'zh-TW' text" once —
  the plugin ships en-US only (localization out of scope); OBS falls
  back to English strings.

Not yet covered by this session (still open on the checklist): the
FORMAL 30-min soak under Record/stream encode load; panic hotkey;
M8 picker (deployed 17:10, after this session).

## Session 2 — formal 30-min soak (pending)

(to be appended by the agent from the owner's next Record session)
