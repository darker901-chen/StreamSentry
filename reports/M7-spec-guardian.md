# M7 Spec-Guardian Audit -- allowlist mode + panic hotkey (SPEC 2.3 + 2.4)

- Date: 2026-07-10
- Auditor: Spec Guardian
- Repo: F:\obsplugin (branch master, HEAD 37d8145; change set = git diff --cached,
  working tree otherwise clean)
- Change set audited (10 files): src/filter.c (mode settings, panic hotkey,
  full-source plate, mask-all branch), src/frame-decide.c/.h (allowlist failure
  direction, mode-transition check, overflow semantics), src/shared-state.h
  (snapshot gains allowlist_mode + mask_all), src/watcher.cpp/.h (shared
  matches_list, allowlist enumeration, overflow flag, list/mode setters),
  data/locale/en-US.ini, ARCHITECTURE.md, tests/frame-decide-tests.c,
  tests/watcher-selftest.cpp
- Authority: CLAUDE.md as amended 2026-07-09; SPEC.md Part 2 sections 2.3, 2.4,
  2.7; owner ruling reports/RULING-2026-07-09-fail-open.md (FINAL, incl. the
  2026-07-10 Application addendum); owner ruling a434b18 (matching semantics);
  carried observation M5-spec-guardian.md section 5 #3 / M6-spec-guardian.md
  section 6 #3 (mask_all honored in blocklist mode) due for closure here.

---

## 1. SPEC 2.3 (allowlist mode) -- line by line

| Requirement | Status | Evidence |
|---|---|---|
| Mode switch in filter settings; blocklist remains the default on upgrade | PASS | filter.c:190-195 (combo, two entries); defaults filter.c:211-217 -- "mode" defaults to "blocklist", "blocklist" default unchanged (ss_watcher_default_blocklist_text()), "allowlist" defaults to "". A pre-M7 settings blob lacks "mode" and so resolves to blocklist: v0.1 behavior preserved |
| Separate storage keys; a mode switch never reinterprets one list as the other | PASS | Distinct keys "blocklist"/"allowlist" read independently (filter.c:113-114); distinct watcher globals g_blocklist/g_allowlist (watcher.cpp:103-104) with separate setters (676-689, 691-699); the mode-visibility callback (filter.c:174-181) only toggles which textbox is SHOWN -- both keys persist untouched, so the round-trip acceptance row is structurally satisfiable |
| Matching semantics identical to blocklist (ruling a434b18) | PASS | One shared matches_list() (watcher.cpp:227-241): case-insensitive substring vs process image name OR window title; both lists parsed by the same parse_multiline_utf8 (trim + lowercase, watcher.cpp:646-674); call sites watcher.cpp:354 (allow) and 358 (block) |
| Every visible, non-cloaked, NOT-matching top-level window reported as SS_RECT_WINDOW | PASS (with V1 exception) | watcher.cpp:348-355 -- !matches_list(...) -> push(SS_RECT_WINDOW) after the standard visible/non-cloaked/non-empty gates (291-301). Exception: a genuine EnumWindows failure silently truncates the report -- violation V1 below |
| Toast + password detection stay active in both modes; approving a process does NOT exempt its toasts | PASS (code) | Toast branch precedes the mode branch and early-returns on an affirmed toast (watcher.cpp:330-346), so the allowlist is never consulted for toast cards; comment states the exemption design (349-353). Password rect appended in publish_tick independent of mode (459-475). No live-toast test possible (banners system-suppressed, reports/M6-toast-probe.txt) -- the approve-explorer-then-fire-toast check belongs on the human matrix (see V3, observation 4) |
| No implicit approvals; empty allowlist = approve nothing | PASS | ss_watcher_set_allowlist has deliberately NO default fallback (watcher.cpp:691-699, comment cites the default-deny promise); g_allowlist initializes empty (104); contrast: empty blocklist falls back to DEFAULT_BLOCKLIST (676-689). matches_list over an empty vector returns false -> everything masked. No shell-surface special cases exist anywhere in enum_proc |
| Rect-budget overflow -> watcher publishes mask_all; allowlist renders one full-source privacy plate (mode default, not a failure) | PASS | enum_proc budget check sets ctx->overflow and stops (watcher.cpp:281-289); dropped password rect also sets it (462-473); published as snap.mask_all (483). frame-decide.c:75-81: allowlist + snapshot mask_all -> out->mask_all, NOT flagged unverified (no chip) -- unit-tested (frame-decide-tests.c:253-262). Render side draws the plate INSTEAD of the target (filter.c:419-431) |
| Blocklist overflow: never drop rects silently | PASS -- closes M5 s5 #3 / M6 s6 #3 | frame-decide.c:82-86: blocklist + mask_all -> keep every published rect (falls through to mapping) + flag "detection overflow (some masks dropped)" -> chip + PROTECTION DEGRADED log via log_mode (filter.c:470-472); unit-tested (frame-decide-tests.c:264-274). The carried observation ("verify mask_all is honored in blocklist mode too") is CLOSED: the cap is no longer a silent drop in either mode |
| Failure semantics in allowlist mode: unverified -> full-source mask-all plate + chip | FAIL (two paths) | Implemented for no-snapshot / stale / mode-transition / geometry-unresolved / mapping-INVALID (frame-decide.c:48-65, 91-95, 104-107; rendered filter.c:419-431). NOT implemented for a genuine EnumWindows failure (V1) nor for the filter-chain-bypass path (V2) |

## 2. SPEC 2.4 (panic hotkey) -- line by line

| Requirement | Status | Evidence |
|---|---|---|
| One hotkey per filter instance via obs_hotkey_register_source; no properties-UI element | PASS | filter.c:139-140 (registered in filter_create with the filter's own source); filter_get_properties adds no panic control (183-207) |
| Name "StreamSentry: mask everything (panic)"; toggle semantics | PASS | data/locale/en-US.ini:10 PanicHotkey string matches SPEC verbatim; callback toggles on key-down only (filter.c:119-132) |
| Unregistered on destroy | PASS | filter.c:165-166, guarded by OBS_INVALID_HOTKEY_ID (registration-failure safe) |
| Engaged -> full-source privacy plate INSTEAD of the target; target never composed | PASS | The panic/mask-all branch (filter.c:419-431) sits BEFORE obs_source_process_filter_begin (441) and returns after drawing -- the target is never entered into the filter chain on panic frames. draw_full_plate (299-310) uses the privacy-plate texture path |
| Chip stacks on top when simultaneously unverified (deliberate action outranks health) | PASS | filter.c:419 ORs panic with dec.mask_all; 421-422 draws the chip whenever dec.unverified -- panic never suppresses the failure notice. Manual row "panic + watcher killed -> plate stays, chip on top" is structurally satisfied (stale -> dec.unverified) |
| NOT persisted across sessions | PASS | State is a volatile bool in the filter struct (filter.c:72), set only by the hotkey callback via os_atomic (126-127); grep confirms no obs_data write of any panic key anywhere; a fresh session bzalloc's it false (136). (OBS persisting the key BINDING is stock OBS behavior and not state) |
| Engage/release logged | PASS | filter.c:128-131 (LOG_WARNING on engage, LOG_INFO on release) |
| Engages within one frame | PASS (structural) | Atomic write -> read at the top of the next filter_video_render (419); exact next-frame visibility is the manual acceptance row (needs OBS; see V3) |
| Interplay with the Enable checkbox | QUESTION Q3 | The !f->enabled early-out (filter.c:365-371) precedes the panic branch: with Enable unchecked, the hotkey still toggles and logs "PANIC engaged ... masking the entire source" (129) while nothing is masked. SPEC 2.4 is silent on this interplay |

## 3. SPEC 2.7 interplay + ARCHITECTURE trigger table vs code

- Evaluation order in code (frame-decide.c): no-snapshot (48) -> stale (53) ->
  mode-transition (59) -> detection_degraded (72, flag only, no return) ->
  watcher mask_all (75) -> empty-rect early-out (88) -> geometry (91) ->
  per-rect mapping (97-109); filter-chain bypass in filter.c:441-448. This
  matches ARCHITECTURE.md:142-153 rows 1-8 exactly, including row 3 (new,
  frame-decide.c) and row 5 (new, blocklist-only, "the 64 rects we do have" --
  SS_MAX_RECTS = 64, shared-state.h:52). First-trigger reason precedence is
  enforced by flag() (frame-decide.c:21-28).
- Mode-transition transient: snapshot mode != filter mode -> rects untrusted,
  chip; allowlist filter additionally mask_all; blocklist filter chip-only.
  Code frame-decide.c:59-65; both directions unit-tested
  (frame-decide-tests.c:199-219). Matches table row 3 and the snapshot-field
  contract (shared-state.h:62-66). PASS.
- Degraded-only exception: implemented (frame-decide.c:67-73 -- degraded does
  NOT set mask_all; confident masks kept, num_mapped preserved) with the
  justification comment in both frame-decide.c:69-71 and frame-decide.h:43-45;
  unit-tested (frame-decide-tests.c:240-251). The exception itself is
  consistent with SPEC 2.3's enumerated mask-all failure list (stale /
  geometry / mapping -- degraded is deliberately absent) -- but the
  justification's premise is challenged in QUESTION Q1.
- ARCHITECTURE.md:155-160 allowlist paragraph ("whenever the frame is
  unverified for any reason EXCEPT #4 ... mask_all") does NOT match the code
  for row 8: see violation V2. All other rows match.
- ARCHITECTURE.md:174-175 claims "the heartbeat is written only at the end of
  a fully successful enumerate-and-publish tick" -- the code publishes and
  beats the heartbeat even when EnumWindows itself fails (V1), so this staged
  claim is not currently true either.
- Panic paragraph (ARCHITECTURE.md:181-186) matches the code (branch
  placement, atomic bool, chip stacking, not persisted, logging). PASS.

## 4. Iron rules against this diff

| Rule | Status | Evidence |
|---|---|---|
| 1 Failure-notice integrity (as amended 2026-07-09) | FAIL | Two failure paths violate it: V1 (EnumWindows failure passes silently -- no chip, no log, fresh heartbeat; in allowlist mode this is a silent pass-through of unapproved windows, the exact direction SPEC 2.3 forbids) and V2 (allowlist filter-chain bypass shows unapproved windows with chip-only instead of failing to the mode's mask-all default). Everything else lands correctly: no blackout anywhere, no guessed positions (mask-all is the mode default / a user action, not a position guess; blocklist overflow deliberately does NOT blanket -- frame-decide.c:82-85 comment), chip remains not user-disableable (no new setting; filter.c:197-199) |
| 1 Never disrupt the output | PASS | Blocklist mode never gains mask_all (the frame-decide fallback macro is allowlist-gated, frame-decide.c:42-46; blocklist overflow keeps per-rect plates + chip); allowlist full plate is the opted-into default (SPEC 2.3); panic is a deliberate user action (SPEC 2.4). The source renders in every other case (filter.c:437-450) |
| 2 Deterministic only | PASS | String compares + rect arithmetic only; no ML/AI anywhere in the diff |
| 3 Opaque masks; confident masks never dropped | PASS | Full-source plate = existing opaque privacy-plate texture, solid alpha-1.0 fallback + log if allocation fails (filter.c:299-310 -- "a full-cover mask is never dropped"); per-rect solid fallback unchanged (459-467); no blur/pixelation/transparency anywhere in the diff; plate-gen.c untouched (opacity remains a tested property) |
| 4 No third-party dependencies | PASS | New APIs are libobs only (obs_hotkey_register_source / obs_hotkey_unregister, util/threading.h os_atomic -- filter.c:22,126-127,139,166); no CMakeLists change in the diff; watcher adds no new Win32 library |
| 5 Scope lock (2.3 + 2.4 only for M7) | PASS | Every code change maps to 2.3, 2.4, or their 2.7 interplay. The M8 picker is absent (properties = enable + mode + two textboxes, filter.c:183-207; no enumerate-windows button). No backlog item (blur, per-app policy, Focus Assist, tray, auto-update, macOS) appears. The mode-visibility callback is settings-UI plumbing for the 2.3 switch, not a feature |
| 6 GPLv2 / original code | PASS | License headers intact on all touched files; new code is idiomatic continuation of this repo written against libobs/Win32 APIs; no imported or adapted foreign code found |

## 5. Threading contract

PASS. Lists + mode live under g_cfg_mutex (watcher.cpp:102-105); the three
setters take it briefly (676-711); the watcher tick captures list pointers and
mode and runs the whole EnumDisplayMonitors + EnumWindows pass inside one
lock_guard scope (423-457), so no half-applied config is ever observed and the
borrowed pointers never escape the lock. The render path gains NO lock: panic
is os_atomic load/set (filter.c:126-127, 419); mode_allowlist is a plain bool
read, same benign pattern as the pre-existing f->enabled (word-sized,
torn-read-free); the snapshot read stays the v0.1 trylock. The hotkey callback
does minimal work (atomic toggle + one log). Heavy work (enumeration,
matching, parsing) stays on the watcher/settings threads.

## 6. CLAUDE.md vs SPEC.md conflict check

No CLAUDE-vs-SPEC conflict. One internal tension between two pieces of
2026-07-09 ruling law surfaced by the new mode: SPEC 2.7's "a mask (or an
allowlist pass-through hole) is drawn only when detection and coordinate
mapping are both confident" vs the Application addendum #3 direction
(cloak-query failure -> treat as cloaked -> skip, watcher.cpp:215-225). In
allowlist mode a skip IS a pass-through hole punched without confidence.
Raised as QUESTION Q2, not a conflict verdict: the addendum predates the
allowlist implementation and its recorded rationale is blocklist-shaped.

## 7. Violations (blocking)

- V1 -- EnumWindows failure passes silently; allowlist default-deny broken
  silently. watcher.cpp:455: the EnumWindows(enum_proc, ...) return value is
  ignored. A genuine enumeration failure (returns FALSE without ctx.overflow
  set -- distinguishable from the legitimate overflow-abort, which also
  returns FALSE) publishes a normal-looking snapshot: fresh heartbeat, no
  detection_degraded, no mask_all, missing rects. In allowlist mode every
  window dropped by the failed pass silently renders unmasked -- violating
  amended iron rule 1 ("no failure path may ... pass silently"), SPEC 2.7
  ("no degradation is ever silent"), and SPEC 2.3's failure direction ("never
  silent pass-through"). In blocklist mode it is a silent possible missed
  mask (pre-existing since v0.1, but this diff edits exactly this block and
  adds the mode whose entire contract the failure breaks). Same category as
  the M6 first-pass monitor-truncation finding (M6-spec-guardian.md 7.1):
  rare trigger, wrong and undocumented failure direction. Fix direction
  (implementer's choice of mechanism, must not be silent): on
  !EnumWindows(...) with ctx.overflow false -> treat as degradation with the
  mode-correct direction (allowlist -> mask_all; blocklist -> chip) and log
  the transition. Also restores the truth of ARCHITECTURE.md:174-175.
- V2 -- Filter-chain-bypass path ignores the allowlist failure direction;
  staged ARCHITECTURE.md contradicts staged code. filter.c:441-448: when
  obs_source_process_filter_begin fails on a healthy allowlist frame with
  unapproved-window plates pending (dec.num_mapped > 0, dec.mask_all false),
  the source is shown via skip_video_filter with chip-only -- the unapproved
  windows render unmasked. SPEC 2.3/2.7 and CLAUDE.md rule 1 direct allowlist
  failures toward the mode's own default (mask-all), and the staged
  ARCHITECTURE.md:155-158 itself promises mask_all for "any reason EXCEPT #4"
  -- row 8 is a reason and is not #4. The table therefore does NOT match the
  code (audit task item 3). No technical excuse exists: draw_full_plate needs
  no filter chain (the mask-all branch at filter.c:419-431 already draws
  without entering it). Fix: in the begin-failure branch, allowlist mode
  draws the full-source plate (+ chip) instead of skipping.
- V3 -- No TESTING.md note in the change set. CLAUDE.md Workflow: "Every
  feature lands with a manual test note appended to TESTING.md." M7 lands two
  features; every prior milestone commit (M2, M3, M4, M5, M6, M6.5 -- see
  git log --name-only -- TESTING.md) included its TESTING.md section, and the
  M6 audit already restated the rule as binding at landing. The staged diff
  has no TESTING.md entry, so the milestone as staged cannot land. The M7
  note must carry at least: the SPEC v0.2 acceptance rows for 2.3/2.4
  (approved app visible + others plated; more than SS_MAX_RECTS windows ->
  single plate; allowlist + watcher killed -> mask-all plate within 500ms;
  mode round-trip preserves both lists; panic next-frame engage/release in
  both modes; panic + watcher killed -> plate stays, chip on top) plus the
  toast-exemption-from-approval check (blocked by the machine's banner
  suppression -- carry it like M6 item 4, with the M6 diagnostic note).

## 8. QUESTIONS for the human (do not guess)

- Q1 -- Degraded-only exception justification is overbroad. The comment
  (frame-decide.c:69-71, frame-decide.h:43-45) justifies not masking-all on
  detection_degraded because "unapproved toast hosts are masked as ordinary
  windows anyway". That premise fails exactly when the toast host is
  APPROVED: SPEC 2.3 itself names approving explorer.exe (taskbar) as the
  expected use, and a real toast is an explorer.exe window -- during
  monitor-data loss the gate cannot affirm it, it falls through to allowlist
  matching, matches the approval, and renders unmasked (chip only). The code
  direction matches SPEC 2.3's enumerated mask-all list (degraded is
  deliberately absent), so this is not ruled a violation -- but the owner
  should either bless and document the residual (approved-toast-host +
  degraded -> toast content visible with chip, same exposure as blocklist
  mode) or direct degraded -> mask_all in allowlist.
- Q2 -- Cloak-query-failure direction in allowlist mode. is_cloaked failure
  -> treated cloaked -> skipped (watcher.cpp:215-225, per ruling addendum #3,
  rationale: a plate over a not-displayed window is a wrong mask). In
  allowlist mode a skipped window is a silent pass-through hole, while SPEC
  2.7's letter requires confidence for "an allowlist pass-through hole".
  Which law governs the new mode? This extends the already-recorded M6.5
  countersign residual (TESTING.md M6.5 item 6) to allowlist.
- Q3 -- Panic vs the Enable checkbox (see section 2): should panic override
  enabled == false, or should the callback avoid logging "masking the entire
  source" when the filter is disabled and nothing will be masked?

## 9. Non-blocking observations

1. Mode is watcher-global while panic is per-filter: two filters in different
   modes leave the mismatched one permanently in "mode transition pending"
   (chip; mask-all if it is the allowlist one). Never silent, fails in the
   safe direction, and last-writer-wins is documented (filter.c:108-111,
   watcher.h:44-46) -- consistent with the v0.1 global-blocklist
   interpretation. Consider one ARCHITECTURE sentence naming the permanent
   (not one-tick) variant of the transition state.
2. filter.c:426 log arm "rect overflow" is unreachable: dec.mask_all is only
   ever set in allowlist mode (blocklist overflow yields chip, not mask_all).
   Cosmetic.
3. filter_update's three watcher setters take g_cfg_mutex independently
   (filter.c:112-114), so one tick can interleave between mode and list
   application. Worst case is a one-tick transient in the mask-more direction
   (fresh allowlist mode with the previous -- initially empty -- allowlist);
   the render-side mode-transition check independently guards the output.
   Acceptable.
4. The 2.3 property "approving a process does not exempt its toasts" is
   verified by code inspection only (watcher.cpp:330-346 ordering); the
   selftest cannot exercise it while banners are system-suppressed. Must ride
   the human matrix via the V3 TESTING.md note.

## 10. Test coverage (audit task item 5)

- tests/frame-decide-tests.c:170-274 -- all required M7 failure directions are
  asserted: healthy allowlist maps per-window plates (no mask_all); stale ->
  mask_all; no-snapshot -> mask_all; mode transition both directions
  (allowlist filter -> mask_all + "mode transition pending"; blocklist filter
  -> chip only, no masks); geometry unresolved -> mask_all; one INVALID rect
  -> mask_all; degraded-only -> chip, NO mask_all, confident mask kept;
  overflow allowlist -> mask_all with NO chip; overflow blocklist -> chip +
  rects kept. All pre-existing calls updated for the new allowlist_mode
  parameter (verified against frame-decide.h:76-78). PASS.
- tests/watcher-selftest.cpp:171-199 -- allowlist leg present: mode on +
  no-match token -> expects an allowlist_mode-stamped snapshot with more
  WINDOW rects than the blocklist baseline OR mask_all, then a clean restore
  on switching back. PASS.
- Not covered (inherent): the V1/V2 paths (the behavior is missing, hence no
  tests -- the fixes must add coverage where feasible); panic (needs OBS --
  manual rows); toast-exemption (blocked, see observation 4).

## Verdict

FAIL -- three violations block the milestone: V1 (silent EnumWindows failure
defeats allowlist default-deny -- amended iron rule 1 / SPEC 2.3 / SPEC 2.7),
V2 (allowlist filter-chain-bypass fails open with chip-only, contradicting
SPEC 2.3 and the staged ARCHITECTURE.md's own trigger-table paragraph), V3
(no TESTING.md note in the change set -- CLAUDE.md workflow rule, binding at
landing per M2-M6.5 precedent). Everything else audited passes: separate-key
storage with v0.1-preserving defaults, shared a434b18 matching,
toast/password independence from approvals, empty-allowlist default-deny,
overflow semantics closing the carried M5/M6 observation #3, panic hotkey
mechanics (registration, unregistration, branch placement before
process_filter_begin, chip stacking, non-persistence, logging), opacity with
solid fallback, no new dependencies, threading contract, determinism, scope
lock (no M8 picker). Three QUESTIONs (Q1-Q3) need human rulings and do not by
themselves block, but Q1's residual must be documented if the current
direction is kept.

---

# ROUND 2 (2026-07-10) -- re-audit after the fix pass

Change set re-audited end-to-end: the 10 round-1 files (6 of them amended:
watcher.cpp ef8decf, filter.c 9e28980, frame-decide.c e5a2ad0, frame-decide.h
81b3d71, frame-decide-tests.c 4491f88, ARCHITECTURE.md cac637f; the other 4 --
shared-state.h, watcher.h, en-US.ini, watcher-selftest.cpp -- byte-identical
to round 1) plus reports/M7-verifier.md (VERIFIED; its addendum pins these
same blob hashes) and this report (round 1 preserved verbatim above, per
M6.5 practice).

## V1 -- RESOLVED

publish_tick now captures the EnumWindows return inside the cfg-lock scope
(watcher.cpp: enum_ok) and distinguishes the two FALSE cases: overflow-abort
(ctx.overflow set -> publish proceeds with mask_all) vs genuine failure
(!enum_ok && !ctx.overflow -> the tick is NOT published: no snapshot, no
heartbeat). The heartbeat therefore only certifies a completed pass; a failed
pass makes the render side unverified within 500ms via the existing stale
trigger -- allowlist mask-all, blocklist chip. Never silent: the failure is
transition-logged immediately (g_enum_failed, LOG_WARNING on engage, recovery
INFO), watcher-thread-only so no new locking. A single-tick transient keeps
the LAST GOOD snapshot active (within the mode's inherent 150ms tick
latency) and still logs -- acceptable under the 500ms staleness definition.
First-tick failure = "no detection snapshot" -> correct directions in both
modes. The new M7 watcher-guarantees paragraph (ARCHITECTURE.md:87-93)
documents exactly this, and the pre-existing heartbeat claim
(ARCHITECTURE.md:183-184) is now true. No automated fixture exists for a
real EnumWindows failure (same class as the M6 monitor-enumeration
triggers) -- verified by inspection and compilation; direction unit-covered
via the stale path it feeds.

## V2 -- RESOLVED

The process_filter_begin-failure branch is mode-aware (filter.c): allowlist
with masks pending draws the full-source privacy plate directly (no filter
chain needed, target never composed via skip) and logs the row-8 DEGRADED
reason; blocklist keeps skip_video_filter + chip (owner ruling direction);
nothing pending -> plain skip in both modes. Default-deny no longer fails
open on this path. ARCHITECTURE.md trigger row 8 updated to the split
behavior. Residual doc nit R2 below.

## V3 -- RESOLVED BY PROCESS (conditional ruling)

The coordinator asked for an explicit ruling on the scribe sequencing.
Verified against the repo, not the claim: .claude/agents/scribe.md defines
the scribe stage as running only AFTER a VERIFIED verifier report and a PASS
from this guardian, folding the evidence into TESTING.md + CHANGELOG.md,
which are then committed together with the milestone. Precedent verified:
git log --name-only shows TESTING.md landing in every milestone commit
(M2, M3, M4, M5, M6, M6.5), and the M6 guardian PASS likewise predated the
scribe note that landed in commit 409964e. Ruling: ACCEPTED as
satisfied-by-process, with binding conditions:
(a) the milestone commit MUST include the scribe's M7 TESTING.md note
    covering: the SPEC v0.2 acceptance rows for 2.3/2.4 (approved app
    visible + all others plated; more windows than SS_MAX_RECTS -> single
    full-source plate; allowlist + watcher killed -> mask-all plate within
    500ms; mode round-trip preserves both list contents; panic
    engage/release on the next frame in both modes; panic + watcher killed
    -> plate stays, chip on top); the toast-exemption-from-approval check
    carried as BLOCKED (banner suppression -- the M6 diagnostic note
    applies); the Q3 residual (panic toggled while Enable is off masks
    nothing until re-enabled -- the log says so); and OWNER COUNTERSIGN
    items for the three M7 derivative rulings (Q1/Q2/Q3 directions -- the
    same mechanism as the M6.5 addendum countersign, TESTING.md M6.5
    item 6).
(b) if the milestone commit lands without that note, the landing violates
    the CLAUDE.md workflow rule and this PASS does not cover it.

## Q1 -- RESOLVED (derivative ruling implemented and recorded)

allowlist + detection_degraded now falls to mask-all: the degraded branch in
frame-decide.c flags the frame AND applies the allowlist fallback (no early
return, so the blocklist V6 guarantee -- confident masks kept on degraded
frames -- is untouched). The overbroad self-sufficiency justification is
gone from frame-decide.c and frame-decide.h; the header paragraph now reads
"unverified for ANY reason -- including detection_degraded" with the
approved-toast-host rationale. Tests: the allowlist+degraded case is flipped
to expect mask_all, and a NEW blocklist+degraded companion case pins V6
(unverified, no mask_all, num_mapped == 1). ARCHITECTURE row 4 and the
allowlist paragraph updated to match. Direction is mask-more inside the
opted-in mode -- consistent with CLAUDE.md rule 1's allowlist exception and
SPEC 2.3 default-deny; recorded in code comments citing guardian M7 Q1 and
in this report; owner countersign rides V3 condition (a).

## Q2 -- RESOLVED (mode-aware; matches SPEC 2.7's letter)

is_cloaked is replaced by a tri-state cloak_state() (watcher.cpp):
CLOAK_YES always skips; CLOAK_UNKNOWN (query failure) skips in blocklist
mode (no mask on doubt -- the ruling addendum #3 direction retained for the
mode it was ruled on) and falls through in allowlist mode (the window is
masked unless approved -- no confidence-less pass-through hole, which is
SPEC 2.7's own sentence about allowlist holes). The comment at the enum
gates documents both directions and cites the rationale. Residual noted as
R3 below; countersign rides V3 condition (a).

## Q3 -- RESOLVED (self-documenting log)

panic_hotkey_cb now appends "(filter currently disabled - takes effect when
enabled)" when panic is toggled while Enable is off (filter.c). The
statement is accurate: the panic branch takes effect on the next rendered
frame once the filter is enabled. The behavioral choice itself (the
explicit Enable off-switch outranks panic; nothing is masked while
disabled) is consistent with ARCHITECTURE's "deliberate non-triggers" and
goes on the TESTING.md note per V3 condition (a).

## Round-2 findings (non-blocking; correct during commit assembly)

- R1 (doc, stale after the Q1 flip): the ARCHITECTURE.md test-infrastructure
  table's frame-decide row still says "degraded-only does not" fail to
  mask-all. That now contradicts the code, the flipped unit test, and the
  same document's own row 4 and allowlist paragraph. One-line correction
  required before the commit is assembled. Doc-only -- per the M6.5 round-3
  precedent it may be applied after this PASS without a new audit round,
  provided no code changes.
- R2 (doc + dead code): trigger-table row 8 says allowlist gets "full-source
  plate ... + chip", but after Q1 every unverified allowlist frame carries
  mask_all and returns in the earlier branch, so the begin-failure branch is
  only reachable on healthy allowlist frames: the dec.unverified chip guard
  inside its allowlist arm is unreachable, and the actual behavior is plate
  + DEGRADED log, no chip. Correct the row wording (and optionally drop the
  dead guard). Not a compliance defect: the full plate is the mode-maximal
  protection and the log line fires.
- R3 (observation -- accepted residual of the Q2 direction): in allowlist
  mode a CLOAK_UNKNOWN window that also matches the toast signature and
  geometry gate receives a toast card even if genuinely cloaked (a card
  over not-displayed pixels, possibly over an approved window's area).
  Requires a triple coincidence (cloak query fails AND the window is
  actually cloaked AND signature+gate match); the direction is mask-more,
  the mode's bias. Record it with the Q2 countersign item.
- R4 (recommendation): for authority-chain symmetry with the M6.5
  Application addendum, add a short M7 entry to
  reports/RULING-2026-07-09-fail-open.md recording the Q1/Q2 derivative
  directions. The minimum record (code comments citing the guardian
  findings + this report + the TESTING.md countersign items) is otherwise
  in place.

## Round-2 rule re-check on the amended files

- Failure-notice integrity: V1/V2 paths now land in the correct
  mode-specific directions with logs; no silent path found in the amended
  hunks; the chip remains not user-disableable. PASS.
- Opacity: unchanged (full plate texture path + solid alpha-1.0 fallback;
  plate-gen untouched). PASS.
- Dependencies: no new APIs beyond libobs/Win32 already in use; the
  verifier pinned CMakeLists.txt as blob-identical to M6.5. PASS.
- Threading: g_enum_failed is watcher-thread-only (transition logging, like
  g_mons_degraded); no new locks; the render path stays lock-free (panic
  atomic + plain bool mode read); enum_ok is captured inside the existing
  cfg-lock scope and checked on the watcher thread. PASS.
- Deterministic: yes. Scope: every round-2 hunk maps to a V or Q fix --
  no new features. PASS.
- CLAUDE.md vs SPEC.md: no conflict. The round-1 section-6 tension is
  dissolved by the Q2 mode-aware direction -- each law now governs exactly
  the mode its rationale was written for.

## Round-2 verdict

PASS -- V1 and V2 are resolved in code and verified line-by-line against
the staged blobs; V3 is resolved by the documented scribe process
(.claude/agents/scribe.md) under the binding conditions (a)/(b) recorded
above; Q1-Q3 are resolved with recorded, principle-derived directions.
No remaining violations. Conditions attached to this PASS:
1. The scribe's M7 TESTING.md note (with the owner-countersign items for
   Q1/Q2/Q3 and the blocked toast-exemption row) must be in the milestone
   commit -- V3 conditions (a)/(b).
2. The R1 and R2 one-line ARCHITECTURE.md corrections must be applied
   (doc-only) before the commit is assembled.
3. This report must be re-staged after this round-2 append so the commit
   carries both rounds verbatim (M6.5 practice).
