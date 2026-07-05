# M2 Spec-Guardian Audit — StreamSentry

- Date: 2026-07-05. Auditor: Spec Guardian (loyalty to CLAUDE.md + SPEC.md only). No source modified.
- Change set: uncommitted working-tree diff vs HEAD = cb5b2b0 (M1 render path).
- Milestone M2: real OS-level detection (COM MTA watcher; Win32 enumeration for toasts + blocklist
  with DWMWA_CLOAKED; UIA focus-changed for password fields; heartbeat; real rects through M1
  mapping; fail-closed on uncertainty).
- Build/test/dependency/architecture facts cited from reports/M2-verifier.md (VERDICT: VERIFIED).

Files inspected: src/watcher.cpp, watcher.h, geom-resolve.c/.h, filter.c, shared-state.h/.c,
data/locale/en-US.ini, CMakeLists.txt, tests/watcher-selftest.cpp, all reports/M2-* + HUMAN_CHECKLIST.md.
Byte-unchanged vs M1 (empty diffstat): plate-gen.c/.h, coord-map.c/.h.

---

## RULING — blocklist semantics (CLAUDE.md AND vs SPEC OR)

CLAUDE.md Architecture line 15: signature for toasts + blocklist = process name AND window class.
SPEC.md Detection section 2 line 9: blocklist = process names / window-title substrings (OR, no class).
Implementation (watcher.cpp:183 toast, :188-197 blocklist): toasts use process AND class; blocklist
uses process-image-name OR title-substring, case-insensitive.

Ruling: DEFENSIBLE RECONCILIATION — NOT a violation. Does not block M2.
1. Iron rule 1 (over-mask, never under-mask; CLAUDE.md lines 7,18) is supreme. A blocklist must
   GUARANTEE coverage; requiring process AND exact class ANDs two failure modes, so a class-name
   drift yields a MISS (under-mask) — the one outcome rule 1 forbids. OR strictly widens coverage.
2. SPEC is the named source of truth for scope/detection (rule 5 defers to it) and its blocklist
   wording has no class requirement; CLAUDE.md line 15 AND is best read as the TOAST signature
   over-generalized. Impl keeps AND where it guards toast false-positives, OR for the blocklist.
3. Documented in M2-DECISIONS.md, surfaced on HUMAN_CHECKLIST.md for owner ruling, reversible
   one-line change. Flag-ambiguity-do-not-guess satisfied.
The two texts do conflict, but this is a mechanism-level conflict resolved toward the higher iron
rule, not a rule-vs-rule deadlock. Ruling: PASS with QUESTION Q1 to human.

---

## Rule 1 — Fail-closed; NOT user-disableable — PASS
- Only masking-off user setting is enabled (filter.c:284-289): false => pass-through (feature off).
  No setting disables fail-closed while enabled. Settings surface = exactly two booleans: enabled,
  debug_kill (verifier Check 4c).
- 500ms threshold is a compile-time non-exposed constant SS_HEARTBEAT_STALE_NS=500000000ULL
  (filter.c:36); not in properties/locale.
- Black-until-proven-alive startup: have_snap starts false (bzalloc); unhealthy = !have_snap ||
  age>threshold (filter.c:296-301). Confirmed live in M2-obs-integration.txt RUN A: FAIL-CLOSED at
  load, watcher up ~140ms later, then cleared. Matches iron rule 1.
- Every new failure path draws black (draw_fail_closed), none fail-open: no-snapshot/stale hb
  (:300-302); geometry unresolved (:311-315); mapping INVALID (:323-327); filter-begin bypass and
  texture-alloc failure both still call draw_fail_closed (diff removed only comments, not blackout).
- geom-resolve.c fails closed on ambiguity: non-monitor_capture (:64-66), base size 0 (:70-71),
  zero monitors (:76-77); does NOT trust an OBS monitor index — resolves only when EXACTLY ONE
  monitor pixel size equals source base (matches!=1 => false, :93-103). Same-res neighbour would
  give wrong origin (under-mask) => refused. Correct.
- Fault injection freezes, never TerminateThread: loop calls publish_tick (sole hb writer) only when
  g_killed==0; killed => neither publish nor beat (watcher.cpp:317-325); flag via InterlockedExchange
  (:434-441). Verifier grep: TerminateThread only in a comment. Kill looks like death => forces black.
- filter_destroy releases a lingering kill (filter.c:129-131) so one instance cannot leave the shared
  watcher frozen for others.

## Rule 2 — Deterministic only; no ML/AI — PASS
Win32 EnumWindows + string/class/process compare + DWM cloaked check + UIA IsPassword property.
No model/inference/classifier/training data anywhere. UIA is an OS accessibility API, not AI.

## Rule 3 — Opaque; no blur/pixelate/mosaic; two styles; black only for fail-closed — PASS
- plate-gen.c/.h byte-unchanged vs M1; M1-verified opaque card (toast) + privacy plate (window/field)
  ship unchanged; plate-gen-tests passes (verifier Check 5). No reversible obfuscation added.
- src/ grep blur|pixelate|mosaic|gaussian|downscale|deblur|depixel: zero hits. Only opacity/alpha,
  in plate-gen.h comments asserting every plate pixel alpha==255.
- Kind-to-style intact (filter.c get_plate_texture): TOAST->card; WINDOW/FIELD->plate.
- Raw black used solely by draw_fail_closed; healthy rects always draw an opaque plate/card else fail
  closed. Reserved-for-fail-closed invariant holds.

## Rule 4 — No third-party deps beyond template + Windows SDK + libobs — PASS
- New CMake link libs: dwmapi, ole32, oleaut32, uuid, user32 — all Windows SDK import libs.
- w32-pthreads is the libobs-shipped shim (OBS::w32-pthreads), not third-party.
- Verifier Check 6 dumpbin: obs.dll, dwmapi, ole32, USER32, w32-pthreads, KERNEL32, MSVCP140,
  VCRUNTIME140(_1), api-ms-win-crt-*. No Qt, no non-Windows/non-libobs library.
- MSVCP140 + VCRUNTIME140 are the MSVC C++ runtime (watcher.cpp uses std::mutex/vector/wstring):
  platform toolchain runtime, same category as the api-ms-win-crt CRT DLLs already in M1 — NOT a
  bundled/vendored library. Acceptable.

## Rule 5 — Scope locked to v0.1; no backlog/premature features — PASS
- Detection = exactly SPEC sections 1-4: toasts, blocklist (defaults = password managers +
  credential dialogs only, watcher.cpp:76-81), password fields (UIA IsPassword), heartbeat.
- ABSENT (SPEC Out of scope): allowlist mode, Focus Assist/auto-DND, content/secret scanning,
  macOS, tray, auto-update, per-app policy, notifications-as-a-feature. DND appears only as a
  test-environment caveat, never integrated behavior.
- Blocklist defaults limited to password managers + credential dialogs (1password/keepass/bitwarden/
  dashlane/lastpass + consent.exe/credentialuibroker/logonui.exe). Dashlane/LastPass = same category.
- Blocklist settings TEXTBOX deferred to M3 (running plugin uses DEFAULT_BLOCKLIST; ss_watcher_set_
  blocklist exists and is unit-tested, not yet wired to properties). REASONABLE STAGING: the textbox
  is UI; the security-critical always-on defaults are present. On HUMAN_CHECKLIST.md item 3.
- debug_kill is developer scaffolding (locale: Developer, M2 scaffolding, removed at M3) and only
  forces fail-closed. Not a shipping feature; M3 removal, as M1 toggles were.

## Rule 6 — GPLv2-or-later headers; original code — PASS
- GPL-2.0-or-later StreamSentry header on every new file: watcher.cpp:1-17, watcher.h:1-17,
  geom-resolve.c:1-17, geom-resolve.h:1-17, tests/watcher-selftest.cpp:1-17. Modified files retain.
- watcher.cpp COM/UIA/Win32 written against Windows SDK interfaces (IUIAutomation,
  IUIAutomationFocusChangedEventHandler, EnumWindows, DwmGetWindowAttribute,
  QueryFullProcessImageNameW). Comments explain SDK-specific build choices. No foreign license/
  attribution headers; no signs of adaptation from another project. Consistent with original code.

## Architecture conformance (CLAUDE.md Architecture (fixed)) — PASS
- ONE watcher thread, COM MTA: single CreateThread(watcher_thread) (:363); CoInitializeEx(nullptr,
  COINIT_MULTITHREADED) (:291), failure logged, no fail-open. Refcounted start/stop (g_refcount,
  :355/:382): ss_watcher_start/stop are per-filter (filter.c:103/:132) but refcount collapses them
  to ONE shared thread. Matches one-watcher-owns-all-detection.
- Cadence in 100-200ms: WATCH_TICK_MS=150 (:58) as WaitForSingleObject timeout.
- Event handler minimal (copy/post/return): HandleFocusChangedEvent reads IsPassword, copies
  BoundingRectangle under a small lock, sets a flag, returns; no enumeration/blocking (:228-250).
  Enumeration runs on the timer, not in the event.
- Shared state = rect list + heartbeat; ss_snapshot correctly slimmed for M2 (geometry fields
  removed, shared-state.h diff): watcher emits SCREEN-space rects only, filter resolves its own
  capture geometry per frame. Clean separation matching the coord chain.
- Render critical section non-blocking + tiny: ss_state_try_read uses pthread_mutex_trylock, returns
  false on contention (shared-state.c:63-72); locked region = one struct copy. On a missed lock the
  last good snapshot is kept (have_snap stays set) but is still heartbeat-age-gated, so a stale keep
  fails closed after 500ms. Correct.
- Coord chain screen->source with over-mask padding: ss_map_screen_rect(&geom, ..., SS_MASK_PAD_PX,
  &out) (filter.c mapping loop); 8-16px padding from M1. Multi-monitor/DPI via exact monitor rect
  (geom-resolve.c), fail closed when ambiguous.
- Toast signature EMPIRICAL and recorded in code comments, as SPEC section 1 mandates: watcher.cpp:
  60-70 records Win11 build 26200 -> explorer.exe + Xaml_WindowedPopupClass, notes it REPLACES the
  SPEC Win10 ShellExperienceHost/CoreWindow example, and that the class over-matches other XAML
  flyouts (over-mask, iron rule 1). This is the mandated verify-and-record step, not a SPEC breach.

## Documented limitations — acceptable v0.1 disclosures, NOT rule/scope problems
- Toast test INCONCLUSIVE under Do Not Disturb: mechanism exercised structurally; DND suppresses the
  on-screen banner (0x0 rect). Reported INCONCLUSIVE (not pass/fail), escalated to checklist item 2
  with a turn-DND-off precondition. Honest.
- Window/game capture geometry unsupported -> fail closed (geom-resolve.c; checklist item 6):
  REQUIRED by iron rule 1 (unresolvable region must black, not guess). SPEC treats window-capture
  origin as a mapping concern, not a v0.1 support promise. Disclosed.
- Dual identical-resolution monitors indistinguishable -> fail closed (geom-resolve.c:82-92): correct;
  guessing would under-mask. Documented v0.1 limitation.
All three resolve toward MORE masking/black, never a silent miss. Limitations, not violations.

---

## Verdict
Every iron rule PASS; architecture PASS; blocklist-semantics conflict resolved as a defensible
reconciliation favoring iron rule 1 (documented, escalated, reversible). Fail-closed integrity holds
on every new path; masking opaque; scope inside v0.1; deps within Windows SDK + libobs + platform
C++ runtime; new files GPL-2.0-or-later and original.

Non-blocking QUESTIONS for the human (do not block M2):
- Q1: confirm blocklist semantics = process-name-OR-title-substring (as implemented, rule-1-safe),
  OR mandate strict CLAUDE.md AND-of-process-and-class. If strict AND, reconcile CLAUDE.md line 15
  with SPEC section 2 so the sources stop conflicting.
- Q2 (tracking): ss_watcher_set_blocklist is built + tested but not wired to a settings textbox;
  ensure M3 lands the SPEC Settings UI blocklist textbox.
- Q3 (verification gap, not a code fault): real on-screen toast masking and UIA password-field-to-
  plate were not machine-verified (DND + no live field focus). Run HUMAN_CHECKLIST items 1-2 before
  release per the SPEC acceptance matrix.

RESULT: PASS
