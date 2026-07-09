# M6 Spec-Guardian Audit -- v0.2 hardening (perf + toast narrowing)

- Date: 2026-07-09 (re-audited same day; see section 7.1)
- Auditor: Spec Guardian
- Repo: F:\obsplugin (branch master, HEAD 6667ae5)
- Change set audited (git diff --cached; working tree otherwise clean):
  - src/watcher.cpp -- PID-to-name cache, per-tick monitor enumeration with
    incomplete-data guard (mons_complete), toast-gate call, always-compiled
    >250ms tick warning, PERF_LOG avg/p99/max + cache stats
  - src/toast-gate.c / .h -- NEW pure module: toast geometry gate (SPEC 2.2)
  - tests/toast-gate-tests.c -- NEW ctest unit for the gate
  - CMakeLists.txt -- wires toast-gate into plugin, ctest, watcher-selftest
  - ARCHITECTURE.md -- M6 as-built updates
  - reports/M6-toast-probe.txt -- NEW probe evidence + OWNER RULING record
- Authority: CLAUDE.md iron rules; SPEC.md Part 2 sections 2.1 + 2.2; owner ruling
  2026-07-09 (reports/M6-toast-probe.txt:19-27, PROVISIONAL gate constants --
  same governance mechanism as ruling a434b18); reports/M5-spec-guardian.md
  section 5 observations #1 and #2 due for closure in this audit.
- Audit history: first pass flagged one non-blocking residual (monitor-list
  truncation fails toward unmask). The implementer fixed it in the staged diff
  the same day; this report reflects the re-audited final change set, and all
  watcher.cpp line references are to that final staged version.

---

## 1. SPEC 2.1 (watcher performance hardening) -- line by line

| Requirement | Status | Evidence |
|---|---|---|
| PID-to-image-name cache, key = PID | PASS | src/watcher.cpp:155-186 (g_pid_cache, cached_proc_image_name); used at watcher.cpp:281 |
| Entry trusted only while PID observed in consecutive ticks; absent one full tick = evicted; reused PID re-queries | PASS | watcher.cpp:188-192 (pid_cache_begin_tick clears seen flags), 194-206 (pid_cache_end_tick erases every unseen entry at the end of that same tick), called around the enum pass at watcher.cpp:397/399 -- a PID unseen for one tick is gone before the next tick begins |
| Residual PID-reuse race documented in code | PASS | watcher.cpp:142-149 -- exact race (reuse within the same tick-to-tick window), why it is bounded, and that title-substring matching (ruling a434b18) is unaffected. CLOSES M5 observation #1. |
| Failed lookups NOT cached | PASS | watcher.cpp:179-184 (empty name never stored) with rationale comment at 151-154 (caching the failure would silently drop proc-name matching; retry cost bounded) |
| Tick instrumentation under STREAMSENTRY_PERF_LOG: avg + max + p99 | PASS | watcher.cpp:452-456 (200-tick buffer, ifdef), 473-497 (sort; avg 482, p99 483 = sorted[197], the correct 99th-percentile index for n=200, max 484); cache hit/eviction stats 485-493; counters ifdef-gated at 161-163, 170-172, 176-178, 199-201. CMake option unchanged, OFF by default (CMakeLists.txt:66-69) |
| >250ms single-tick warning ALWAYS compiled | PASS | watcher.cpp:464-471 -- outside every #ifdef; threshold TICK_WARN_NS = 250,000,000 ns (watcher.cpp:62-65) = half the 500ms stale threshold; a code constant, not a user setting |
| Thread priority NOT applied (measure-first; approved second lever) | PASS | comment at the CreateThread site, watcher.cpp:541-545; grep confirms zero SetThreadPriority calls in src/ (the only THREAD_PRIORITY mention is that comment) |
| Heartbeat semantics untouched; ss_state_touch_heartbeat stays caller-free | PASS | heartbeat still written only together with a completed publish (watcher.cpp:420-421, logic untouched by the diff); kill path still freezes publish+heartbeat (watcher.cpp:499-501); grep: ss_state_touch_heartbeat has only its declaration (src/shared-state.h:67) and definition (src/shared-state.c:54), zero call sites; shared-state.c/.h not in the change set |
| Acceptance: 30-min soak, zero stale events, p99 <= 50ms recorded in reports/ | PENDING (owner acceptance pass) | ARCHITECTURE.md:207-211 records it as on the owner soak checklist; not satisfiable by code audit -- remains binding for the v0.2 ship |

## 2. SPEC 2.2 (toast-match narrowing by geometry) -- line by line

| Requirement | Status | Evidence |
|---|---|---|
| Process+class signature kept as the base gate | PASS | watcher.cpp:298 unchanged AND-match (proc == TOAST_PROC && cls_l == TOAST_CLASS); signature comment watcher.cpp:67-77 |
| Geometry gate: (a) toast spawn band of some monitor, (b) plausible dimensions | PASS | src/toast-gate.c:73-116 per-monitor rule (size envelope 88-95, monitor-overlap requirement 97-106, right-edge band 108-113); any-monitor semantics toast-gate.c:118-128; monitor rects refreshed every tick (watcher.cpp:386-396 via mon_enum_proc 227-246), with incomplete data forced to an empty list so the gate stays in its mask direction (see section 7.1) |
| Constants verified empirically per build family, recorded in code comments | PASS under OWNER RULING 2026-07-09 | Toast banners are system-suppressed on the dev machine (full diagnosis: reports/M6-toast-probe.txt:7-49). The ruling at M6-toast-probe.txt:19-27 authorizes PROVISIONAL documented-metrics constants for M6 only and defers live calibration + v0.1 signature re-verification on build 26200.8655 to the owner acceptance pass. Code marks the constants PROVISIONAL and cites the ruling (toast-gate.c:23-31); ARCHITECTURE.md:194-204 records it. The sequencing amendment is accepted as governing authority (same mechanism as a434b18); every condition the ruling keeps binding is verified below |
| Generous bounds, with derivations in comments | PASS | toast-gate.c:33-48 (396-DIP basis; width [200,1000] px spans 50-250% scale, wider than the supported 100-250% range; height [60,1200]; 160px edge band = 4x the observed inset at 250% scale; the 60%/90% monitor-fraction caps are scale-invariant, so no real toast can trip them on any monitor at least ~660 DIP wide -- Windows minimum is 800). Constants toast-gate.c:60-66; same derivations recorded in M6-toast-probe.txt:89-109 |
| Spawn band spans the FULL right edge (top-right and bottom-right anchors) | PASS | toast-gate.c:108-113 -- the band constrains only the right edge (one-sided, so slide-in overhang past the monitor edge still passes); vertical position free across the whole monitor height (overlap check only, 97-106). Test fixtures cover both anchors: tests/toast-gate-tests.c:61 (bottom-right), 63 (top-right), 69 (slide-in) |
| Over-mask-safety argument required in code comments | PASS | toast-gate.c:50-58 -- states the false-positive-to-false-negative trade explicitly, why each bound is safe, and that the real-toast acceptance row remains binding. Module contract restated in toast-gate.h:19-29. CLOSES M5 observation #2 for the code-side conditions; the live real-toast acceptance row is deferred to the owner pass per the ruling and stays binding |
| Uncertainty classifies as toast (mask) | PASS | toast-gate.c:75-79 (NULL/non-finite/degenerate monitor), 84-85 (degenerate window -- unreachable from the watcher, which drops empty rects at watcher.cpp:262-264, but safe for standalone callers), 120-122 (no monitor data at all); watcher additionally forces num_mons = 0 on ANY incomplete monitor enumeration (watcher.cpp:388-396, direction comment 390-394); watcher-side comment watcher.cpp:293-297 |
| Gate failure only removes toast-card styling; window still reaches block/allowlist matching; fail-closed never affected | PASS | watcher.cpp:298-319 -- on gate fail there is no early return; the window falls through to the blocklist loop (310-319). The gate touches classification only; health = heartbeat publish, untouched (section 1). A rejected window loses the toast card but the pipeline never fails open at the health level |
| Test coverage of the failure direction | PASS | tests/toast-gate-tests.c:111-127 -- NULL monitor list, zero count, NULL monitor, degenerate (0x0) monitor, NaN monitor: all must return true (mask). Positives at documented metrics across 100-200% DPI incl. secondary/negative-origin monitor (59-71); recorded flyover shapes rejected (73-100); the one admitted flyover shape (tray-overflow-sized, right edge) is asserted as deliberate over-mask so any future tightening is a conscious act (80-84); multi-monitor any-semantics (102-109). Wired as ctest: CMakeLists.txt:80,83. The empty-list cases (112, 115) are exactly the path the watcher takes on incomplete enumeration |
| Acceptance: no plate storm; real toast still masked before readable | PENDING (owner acceptance pass, per ruling) | M6-toast-probe.txt:22-27; note also probe finding M6-toast-probe.txt:58-62 -- the shell window taxonomy changed on .8655, so the original storm sources must be re-checked at acceptance too |

## 3. Iron rules against this diff

| Rule | Status | Evidence |
|---|---|---|
| 1 Fail-closed integrity; not user-disableable | PASS | Every new failure path lands in over-mask or leaves health untouched, never fail-open: monitor enumeration failing, truncating at SS_MAX_MONITORS, or losing any single monitor to a GetMonitorInfoW failure -> num_mons forced to 0 -> gate returns true -> every signature match gets the toast card (watcher.cpp:388-396 + mon_enum_proc 230-233, 242-244; toast-gate.c:120-122) -- identical to v0.1 over-mask behavior; cache lookup failure -> empty, uncached name -> title matching still applies, v0.1 parity (watcher.cpp:314-315); heartbeat/publish/kill/render health logic untouched by the diff (watcher.cpp:420-421, 499-501; src/filter.c not in the change set). No new setting of any kind -- TICK_WARN_NS and the gate constants are compile-time constants |
| 2 Deterministic only | PASS | The gate is pure arithmetic on rects (toast-gate.c); no ML/AI anywhere in the diff |
| 3 Opaque masking | NOT-APPLICABLE | No mask-rendering change: the diff changes which rect KIND a window gets, never how masks are drawn; both kinds remain the existing opaque plates; the raw-black fail-closed reservation is untouched (plate-gen.c, filter.c not in the change set) |
| 4 No third-party dependencies | PASS | toast-gate.c includes only toast-gate.h + math.h; toast-gate.h only stdbool/stddef + in-repo coord-map.h. The watcher's new calls EnumDisplayMonitors/GetMonitorInfoW are user32, already linked before this diff (CMakeLists.txt:60 plugin, :97 selftest -- neither line changed). No new find_package / target_link_libraries (the CMake diff adds only sources and the test executable) |
| 5 Scope lock (v0.2; sections 2.1+2.2 only for M6) | PASS | Every code change maps to 2.1 (cache, instrumentation, warning, priority comment) or 2.2 (gate, monitor enumeration + completeness guard, tests); ARCHITECTURE.md changes are as-built documentation; M6-toast-probe.txt is evidence. No backlog item touched; no new feature; no new setting |
| 6 GPLv2 / original code only | PASS | GPL-2.0-or-later header on all three new files (toast-gate.c:1-17, toast-gate.h:1-17, toast-gate-tests.c:1-17). Constants derived from documented Windows toast metrics plus this repo's own probe record (toast-gate.c:33-37; M6-toast-probe.txt:89-109); no imported or adapted code found |

## 4. Threading contract

- PASS. The cache and monitor enumeration run only inside publish_tick on the
  watcher thread; the cache is explicitly watcher-thread-only with no lock
  (watcher.cpp:136-137), which is correct because EnumWindows/EnumDisplayMonitors
  callbacks run synchronously on the calling thread.
- No new locks (ARCHITECTURE.md:71 claim verified: grep shows the same three
  mutexes as v0.1 -- g_mutex, g_cfg_mutex, g_pw_mutex).
- Render side untouched: filter.c / shared-state.c are not in the change set; the
  render read remains the v0.1 trylock. g_cfg_mutex (whose hold now additionally
  spans the cheap EnumDisplayMonitors + completeness guard, watcher.cpp:382-400)
  is contended only by the settings-update path (watcher.cpp:583) and blocklist
  init (520), never by the render callback.

## 5. CLAUDE.md vs SPEC.md conflict check

None. The one deviation from SPEC 2.2's verify-empirically-first sequencing is
governed by the recorded owner ruling (M6-toast-probe.txt:19-27), which explicitly
keeps every mitigation condition binding -- and section 2 above verifies each of
those conditions is present in the code. CLAUDE.md architecture bullet 1 ("v0.2
adds a geometry gate") is now true as built.

## 6. M5 section-5 observation closure

1. PID-cache residual race must be documented in code -- CLOSED
   (watcher.cpp:142-149).
2. Toast-narrowing mitigation conditions -- CLOSED for everything checkable in
   code (generous bounds + derivations, full-right-edge band, safety argument,
   uncertainty direction, test coverage -- section 2 above); the live real-toast
   acceptance row is deferred to the owner acceptance pass by the 2026-07-09
   ruling and remains binding.
3. mask_all in blocklist mode -- M7 scope, not due in this audit; carried forward.

## 7. Findings from this audit

1. Monitor-list truncation edge -- FLAGGED first pass, FIXED in M6, now CLOSED.
   First-pass finding: SS_MAX_MONITORS = 16 truncated the enumeration and a
   GetMonitorInfoW failure silently skipped a monitor; with num_mons > 0 a
   signature-matched real toast on an unlisted monitor would fail the gate's
   overlap test, lose its toast card, fall to the blocklist (where explorer.exe
   is normally unlisted) and go unmasked -- inverting the
   uncertainty-classifies-as-toast direction in 17+-monitor or hot-unplug-race
   configurations, undocumented. Fix verified in the final staged diff:
   - EnumCtx gains mons_complete (watcher.cpp:224), set false when the cap
     truncates the list (230-233) and when GetMonitorInfoW fails for any
     monitor (242-244);
   - publish_tick resets it per tick (387) and forces num_mons = 0 whenever
     EnumDisplayMonitors returns FALSE or the list is incomplete (388-396),
     with the failure-direction rationale in a code comment (390-394).
   Direction confirmed: an empty monitor list makes ss_toast_geom_plausible_any
   return true (toast-gate.c:120-122), so on any incomplete enumeration every
   signature-matched window is masked as a toast -- exactly the v0.1 over-mask
   behavior, iron rule 1 direction. No new issue introduced: exactly-16-monitor
   systems do not falsely truncate (the cap branch fires only on a 17th
   callback); the guard runs on the watcher thread inside the existing lock
   scope; no new locks, dependencies, settings, or render-side impact; the
   healthy path (complete list) is unchanged from the first-pass audit.
2. RTL coverage boundary (spec-compliant; note only). SPEC 2.2 specifies the band
   "spans the full right edge" and the gate implements exactly that; on RTL
   Windows systems toasts anchor to the LEFT work-area edge (noted at
   toast-gate.c:36, "LTR systems"). Extending coverage would be a spec change,
   not a code fix; record as a known limitation if RTL users ever matter.
3. Binding items deferred to the owner acceptance pass (per the 2026-07-09 ruling
   and the 2.1/2.2 acceptance rows): gate-constant calibration against a live
   banner; toast-signature re-verification on 26200.8655 (the probe shows the
   shell window taxonomy changed: M6-toast-probe.txt:58-62); the real-toast-masked
   and no-plate-storm rows; the 30-min soak with p99 <= 50ms recorded in reports/.
   Additionally, the TESTING.md diagnostic note promised by the probe record
   (M6-toast-probe.txt:82-87) is not in this change set -- it must land with the
   milestone per the CLAUDE.md workflow rule ("every feature lands with a manual
   test note appended to TESTING.md").

## Verdict

PASS -- all checkable 2.1/2.2 requirements and all six iron rules pass on the
final staged change set; both due M5 observations are closed; the one residual
flagged on the first pass (7.1) was fixed in-milestone and re-verified in the
over-mask direction, leaving zero open code findings; no CLAUDE/SPEC conflict; no
unresolved ambiguity requiring a human ruling. The deferred acceptance items in
7.3 remain binding gates for the v0.2 ship, per the owner ruling recorded in
reports/M6-toast-probe.txt.
