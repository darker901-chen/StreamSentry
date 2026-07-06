# M5 Spec-Guardian Audit — v0.2 kickoff (docs + governance)

- Date: 2026-07-06
- Auditor: Spec Guardian
- Repo: F:\obsplugin (branch master, last commit 0417d70)
- Change set audited (git diff --cached; working tree otherwise clean):
  - ARCHITECTURE.md — new (196 lines, as-built v0.1 documentation)
  - SPEC.md — retitled; Part 1 gains one inline as-built note; Part 2 (v0.2 additions) appended
  - CLAUDE.md — rule 5 scope bump v0.1 -> v0.2; "What this is" mentions allowlist + panic;
    architecture bullet 1 split into toast (AND class) vs list (OR title) matching
  - .gitignore — "!ARCHITECTURE.md" whitelist line only
  - No src/, tests/, or CMake changes staged. Confirmed via git diff --cached --stat.
- Authority: reports/V02-PLAN.md (owner-approved 2026-07-05, "全部作") and commit a434b18
  (owner ruling FINAL: blocklist = process-name OR title-substring).

---

## 1. Milestone-specific audit: edits to the law documents themselves

### 1a. All six iron rules present, none weakened
| Rule | Status | Evidence |
|---|---|---|
| 1 Fail-closed, not user-disableable | PASS — byte-identical | CLAUDE.md:7; diff shows line untouched |
| 2 Deterministic only, no ML/AI | PASS — byte-identical | CLAUDE.md:8 |
| 3 Opaque masking, no blur/pixelate; raw black reserved for fail-closed | PASS — byte-identical | CLAUDE.md:9 |
| 4 No third-party deps | PASS — byte-identical | CLAUDE.md:10 |
| 5 Scope lock | PASS — the one owner-approved change: v0.1 -> v0.2 (V02-PLAN.md:39). Backlog list is a superset of the old one (adds "Focus Assist integration"; removes nothing) | CLAUDE.md:11 |
| 6 GPLv2 / original code only | PASS — byte-identical | CLAUDE.md:12 |

### 1b. CLAUDE.md architecture line vs owner ruling a434b18 vs as-built code
- New line (CLAUDE.md:15): toast signature = process AND class (+ v0.2 geometry gate);
  block/allowlist = case-insensitive substring vs process image name OR window title.
- Ruling a434b18 ("blocklist stays process-OR-title (final) ... keep the OR match
  semantics as implemented"): matches. PASS.
- As-built code: src/watcher.cpp:183 toast requires proc == TOAST_PROC && cls_l ==
  TOAST_CLASS (AND); src/watcher.cpp:188-197 blocklist entry matches process image name
  OR window title, case-insensitive via to_lower (watcher.cpp:100-104,157,166,170).
  Matches. PASS.
- Note: the OLD CLAUDE.md line ("toasts + blocklist (signature = process name AND window
  class)") contradicted the ratified as-built OR matching; this edit RESOLVES a standing
  CLAUDE-vs-code tension in the direction the owner ruled. Strengthening, not weakening.

### 1c. CLAUDE.md / SPEC.md mutual consistency
- No conflicts found. "What this is" (CLAUDE.md:4) matches SPEC Part 2 sections 2.3/2.4;
  architecture bullet 1 matches SPEC 2.2/2.3; SPEC Part 2 preamble (SPEC.md:72-73)
  re-binds all v0.1 iron rules explicitly. PASS.

## 2. SPEC.md Part 2 vs the approved plan (scope lock)
- V02-PLAN.md milestone table (V02-PLAN.md:37-42): M6 = perf hardening + toast
  narrowing; M7 = allowlist + panic hotkey; M8 = picker + ship. SPEC Part 2 maps
  2.1->M6, 2.2->M6, 2.3->M7, 2.4->M7, 2.5->M8. Chromium investigation (2.6) =
  plan finding 5 ("investigate ... if time", V02-PLAN.md:27-28), marked
  documentation-only, non-gating (SPEC.md:203-206). Nothing in Part 2 exceeds the plan. PASS.
- Out-of-scope section (SPEC.md:208-216) keeps every v0.1 exclusion not explicitly
  pulled in; Focus Assist explicitly stays backlog ("NOT in the approved plan"). PASS.
- Part 1 preserved as historical record: diff shows only the title/preamble hunk and
  one inline as-built note on detection item 1 (superseded toast signature — matches
  src/watcher.cpp:60-68 code comment); old lines 12-55 byte-identical. PASS.
- Labeling note (non-blocking): SPEC Part 2 counts the panic hotkey as one of the
  "five items" with Chromium as a separate stretch; V02-PLAN's findings list numbers
  Chromium as finding 5 while panic lives in the M7 milestone row. The substantive
  scope is identical either way (panic is explicitly in the approved milestone table:
  OBS hotkey "mask everything now" toggle, V02-PLAN.md:41). No action required.

## 3. Standing rules against the new SPEC Part 2 text

### 3a. Fail-closed integrity — PASS
- 2.1: heartbeat written only with a completed publish; ss_state_touch_heartbeat
  stays caller-free (SPEC.md:94-97). Verified as-built: zero call sites today
  (grep: only declaration shared-state.h:67 + definition shared-state.c:54).
- 2.3: "Fail-closed semantics unchanged in allowlist mode ... full black + banner,
  exactly as v0.1" (SPEC.md:152-155). Rect overflow is over-mask (plate), never a
  silent drop and never a health bypass (SPEC.md:147-151).
- 2.4: "if both apply, fail-closed (black + banner) wins" (SPEC.md:170-171);
  panic not persisted — "fail-closed — not panic — is the safety net" (SPEC.md:172-174).
  Acceptance rows encode both (SPEC.md:227, 229).
- No text anywhere makes fail-closed user-disableable; Part 1 "No option to disable
  fail-closed behavior" (SPEC.md:41) untouched; threshold constant remains
  non-configurable (src/filter.c:33-36).

### 3b. Masking opacity — PASS
- 2.3 overflow: one full-source privacy plate, "opaque, lock + 'Hidden' — NOT raw
  black, which stays reserved for fail-closed" (SPEC.md:148-151).
- 2.4 panic: full-source privacy plate, opaque (SPEC.md:166-168).
- No blur, pixelation, or transparency anywhere in the new text. Raw-black reservation
  restated twice. Full-source use of the existing privacy-plate style is not a new
  mask style and complies with iron rule 3.

### 3c. Scope lock — PASS (see section 2; nothing beyond the five approved items + non-gating 2.6)

### 3d. Dependency rule — PASS
- No CMake/dependency changes staged. 2.5 picker constrained to stock OBS properties,
  "no custom Qt dependency — iron rule 4" (SPEC.md:189-191). 2.4 uses libobs
  obs_hotkey_register_source (SPEC.md:163-164). 2.6 requires "deterministic,
  dependency-free activation" or documentation-only (SPEC.md:203-205).

### 3e. Origin rule — PASS
- Change set is documentation of this repo's own code; no imported/adapted code.

### 3f. Threading contract — PASS (docs only; no render-path code changed)
- 2.1 explicitly keeps heavy work (process-name queries, caching) on the watcher and
  forbids heartbeat manipulation mid-tick. ARCHITECTURE.md correctly documents the
  trylock-only render read (see section 4).

### 3g. Deterministic only — PASS
- 2.2 geometry gate is deterministic (spawn band + size envelope, empirically verified
  per build family, recorded in code comments — same discipline as the v0.1 signature).
  2.6 forbids speculative code. No ML/AI anywhere.

## 4. ARCHITECTURE.md as-built accuracy (spot-check vs source)
| Claim | Verified at | Status |
|---|---|---|
| Heartbeat stale threshold 500ms, not user-configurable; future heartbeat also fails closed | src/filter.c:36, 299-302 (future => age=UINT64_MAX) | TRUE |
| Six fail-closed triggers with reasons | filter.c:302 (no snapshot), 301-302 (stale), 313-315 (geometry), 325-327 (mapping), 356-359 (chain bypass), 366-371 (plate alloc, mid-draw black-over) | TRUE |
| All rects mapped BEFORE target render | filter.c:305-332 precede process_filter_begin at 356 | TRUE |
| Non-triggers: enabled==false and zero-size target skip filter; empty-rect healthy = pass-through | filter.c:284-294, 311 | TRUE |
| Render read is trylock-only; lost read keeps last snapshot, heartbeat still governs | src/shared-state.c:63-72; filter.c:296-301 | TRUE |
| Watcher: 150ms tick; toast explorer.exe AND Xaml_WindowedPopupClass; blocklist OR title; DWMWA_CLOAKED + visible + non-empty gates | src/watcher.cpp:58, 69-70, 183, 188-197, 123-129, 143-153 | TRUE |
| g_cfg_mutex held for the whole EnumWindows pass; g_pw_mutex for UIA rect; g_mutex for refcount | watcher.cpp:260-265, 236 + 268-279, 87-98, 374, 399, 484 | TRUE |
| Heartbeat written only with a full successful publish; touch_heartbeat has no callers | watcher.cpp:281-287; grep: zero call sites | TRUE |
| Refcounted start/stop; CreateThread failure rolls back refcount (fails closed via "no snapshot"); 3s join | watcher.cpp:372-393, 395-417 | TRUE |
| UIA handler minimal work on COM threads; UIA init failure degrades secondary guard with warning | watcher.cpp:226-250, 296-309 | TRUE |
| Fault-injection kill freezes publish AND heartbeat | watcher.cpp:321-345, 473-480 | TRUE |
| SS_MAX_RECTS=64; enum pass stops at cap | src/shared-state.h:52; watcher.cpp:140-141 | TRUE |
| geom-resolve: monitor_capture only; exactly-one monitor size match else fail (incl. identical-resolution ambiguity) | src/geom-resolve.c:54-66, 93-103 | TRUE |
| coord-map: intersect, scale, pad, clamp; INVALID on degenerate/non-finite; SS_MASK_PAD_PX=12 | src/coord-map.c:31-56, 61-77; filter.c:38-40 | TRUE |
| plate-gen and coord-map are pure (no OBS, no Windows); opacity is a tested property | plate-gen.c includes only plate-gen.h/stdlib/string; coord-map.c only coord-map.h/math.h; plate-gen.h:63,67 | TRUE |
| 8-entry LRU plate cache, 16px buckets min 32, styles collapse to two; banner cached, skipped below 160x60 | filter.c:48, 190-231, 199, 239-256, 248 | TRUE |
| ctest targets coord-map-tests/plate-gen-tests; watcher-selftest NOT add_test, links watcher.cpp+shared-state.c with stubs | CMakeLists.txt:76-79, 84-92 | TRUE |
| Perf sampling only under STREAMSENTRY_PERF_LOG CMake option, OFF by default | CMakeLists.txt:64-66; ifdefs in filter.c:334-348, watcher.cpp:317-339 | TRUE |
| M3 perf numbers (0.55% core / 0.03us frame / ~321MB soak) | consistent with reports/M3-perf.md and V02-PLAN.md:66-67 | TRUE |

No materially false claim found. ARCHITECTURE.md is faithful to the as-built v0.1 source.

## 5. Observations (non-blocking, for the M6/M7 implementers)
1. 2.1 PID-to-name cache accepts a documented residual race (PID reuse across a
   one-tick gap can serve a stale name). Owner-approved trade-off with a conservative
   eviction rule; health semantics untouched. Implementer must document the race in
   code, per spec.
2. 2.2 narrowing converts false-positive plates into false-negative risk; the spec's
   mandatory mitigations (generous bounds, full-right-edge band, code-comment safety
   argument, real-toast acceptance row) are the compliance conditions — the M6 audit
   will check them.
3. Pre-existing v0.1 behavior: rects beyond SS_MAX_RECTS are silently unreported
   (watcher.cpp:140-141) — honestly documented in ARCHITECTURE.md; 2.3's mask_all
   requirement is the fix where the cap becomes reachable. The M7 audit should verify
   mask_all is honored in blocklist mode too if the cap can be hit there.

## Verdict

**PASS** — all rules checked pass; no violations; no unresolved ambiguities requiring
a human ruling. M5 may proceed to scribe/commit.
