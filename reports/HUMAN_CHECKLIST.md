# HUMAN_CHECKLIST — items automation cannot verify

Accumulated across milestones; do these in one manual pass at the end.
Everything here has programmatic evidence for the underlying mechanism —
these entries are only about what a human eye must confirm on screen.

## M1 — plate visuals (~2 min)

Setup: OBS → 場景集合 → 選 "StreamSentryM1"(已存在)。The test filter
on source "TestColor" has both DEBUG toggles on, so it blacks out ~0.5s
after載入 — first uncheck "DEBUG: simulate watcher stall" in the filter
properties to see the plates.

1. [ ] Toast card (top-right): rounded dark card, bell icon, readable
       "Notification hidden" label — looks like a native notification
       placeholder, not a glitch.
2. [ ] Privacy plate (lower-middle): dark plate, lock icon, "Hidden".
3. [ ] Both are fully opaque — nothing of the source shows through
       (unit tests assert alpha=255; this checks the on-screen result).
4. [ ] Re-check "DEBUG: simulate watcher stall": within ~0.5s the whole
       source goes black with the red "Privacy guard: detection
       unavailable - output blocked" banner (log evidence: age 519ms).
5. [ ] Switch 場景集合 back to "無標題" when done.
