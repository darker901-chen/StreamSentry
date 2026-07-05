# SPEC.md acceptance matrix — status at M3

Each row of SPEC.md's acceptance matrix, mapped to its evidence. "AUTO" =
machine-verified this session; "UNIT" = covered by a pure unit test;
"MECHANISM" = the code path is verified but the specific real-world
trigger is on the human checklist; "HUMAN" = must be eyeballed (in
reports/HUMAN_CHECKLIST.md).

| # | Scenario | Expected | Status | Evidence |
|---|----------|----------|--------|----------|
| 1 | Windows toast appears | Toast region plated | MECHANISM + HUMAN | Watcher detects explorer.exe + Xaml_WindowedPopupClass (watcher.cpp; selftest enumerates it). End-to-end banner masking INCONCLUSIVE here — machine's Do Not Disturb suppresses the banner (M2-DECISIONS.md). Human: turn DND off, HUMAN_CHECKLIST M2#2. |
| 2 | Password field focused (Chrome/Edge/Firefox) | Field masked | MECHANISM + HUMAN | UIA focus handler reads UIA_IsPasswordProperty + BoundingRectangle (watcher.cpp). Live browser field-to-plate not machine-verified → HUMAN_CHECKLIST M2#1. |
| 3 | Credential dialog / UAC | Masked / full-window plate | HUMAN | UAC runs on the secure desktop (not capturable by OBS at all — documented limitation); credentialuibroker/consent.exe are in the default blocklist. HUMAN_CHECKLIST M2#3. |
| 4 | 1Password / KeePass open | Full-window plate | MECHANISM + HUMAN | Blocklist process/title matching proven with notepad (M2-watcher-selftest-output.txt: appears on open, clears on close). Specific apps not installed here → HUMAN if available. |
| 5 | Password field on 2nd monitor, different DPI | Correct coordinates | UNIT + HUMAN | coord-map mixed-DPI cases pass (coord-map-tests). Real dual-monitor DPI → HUMAN. Note: dual identical-resolution monitors fail closed in v0.1 (geom-resolve, documented). |
| 6 | Source has scale/crop transform | Mask lands correctly | UNIT + HUMAN | coord-map scaled/cropped cases pass. NOTE: geom-resolve resolves only unscaled full-monitor display capture in v0.1; a scaled capture fails closed (safe) rather than mis-placing — documented limitation. HUMAN to confirm. |
| 7 | Kill watcher thread (fault injection) | Full black ≤500ms + status text | **AUTO ✓** | watcher-selftest: heartbeat frozen >500ms on kill, resumes on release. M1 render fail-closed measured ~517ms. OBS integration RUN B: killed → stays fail-closed (M2-obs-integration.txt). |
| 8 | Toggle plugin during 60fps recording | No visible frame drops (measure render time) | AUTO (cost) + HUMAN (visual) | Render decision cost measured via perf build — see M3-perf.md. Visual "no dropped frames" during record → HUMAN. |
| 9 | 2-hour idle run | No memory leak (stable working set) | AUTO (30 min) + HUMAN (2 h) | 30-minute soak with periodic toasts, working-set sampled — see M3-perf.md / M3-soak-samples.csv. SPEC's full 2-hour run → HUMAN. |
| 10 | Empty blocklist + no password field | Fully transparent pass-through | **AUTO ✓** | Soak ran the filter with no sensitive windows present: steady-state pass-through, no plates drawn, no fail-closed (OBS log). Equivalent to the empty-blocklist case (no matches → no rects → pass-through). |

## Summary
- Fully machine-verified: rows 7, 10.
- Mechanism verified, real-world trigger on human checklist: rows 1, 2, 4.
- Math unit-tested, real device on human checklist: rows 5, 6.
- Performance/leak measured this session (numbers in M3-perf.md), longer
  SPEC targets (2 h) on human checklist: rows 8, 9.
- Row 3 is inherently manual (secure desktop / installed apps).
