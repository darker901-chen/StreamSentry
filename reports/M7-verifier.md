# M7 Verifier Report — allowlist mode + panic hotkey (SPEC §2.3 + §2.4)

- Run: 2026-07-10, 11:22–11:27 local, local machine (Windows 11 Home 10.0.26200)
- Tree under test: HEAD `37d8145` + staged M7 diff (10 files, +470/−70), no unstaged changes
- Toolchain: CMake preset `windows-x64-local` (generator "Visual Studio 17 2022" x64),
  config RelWithDebInfo, dependency cache `.deps` present (obs-studio-31.1.1)
- Process note: the Claude Code process restarted after scope spot-checks but BEFORE any
  build/test step had started; tree identity was re-verified unchanged after the restart
  (same HEAD, same staged blobs, 0 unstaged), and the entire build/test sequence below
  ran as one uninterrupted post-restart pass. No partial results were reused.
- **Verdict: VERIFIED**

## 0. Scope of the staged diff (verified present)
Staged files (all 10, working tree == index; `CMakeLists.txt` blob `a3e94f5` unchanged
from M6.5 — "no CMake changes" confirmed):
`src/watcher.cpp` `0c06de7` · `src/watcher.h` `2ffbd95` · `src/filter.c` `66ba506` ·
`src/frame-decide.c` `1deecd5` · `src/frame-decide.h` `12263a8` ·
`src/shared-state.h` `43340e2` · `data/locale/en-US.ini` `f4d1e5e` ·
`tests/frame-decide-tests.c` `d49d673` · `tests/watcher-selftest.cpp` `5cba92e` ·
`ARCHITECTURE.md` `4567c8b`.

Spot-checks against the coordinator's description (all found in the staged diff):
- watcher: `g_allowlist` vector + `g_mode_allowlist` global under `g_cfg_mutex`;
  shared `matches_list()` / `parse_multiline_utf8()` refactor; allowlist branch in the
  enum proc (every visible non-cloaked window that is not approved -> rect);
  rect-budget overflow sets `ctx.overflow` -> `snap.mask_all`; snapshot stamped with
  `allowlist_mode`; `ss_watcher_set_mode_allowlist()` / `ss_watcher_set_allowlist()`
  (empty allowlist deliberately approves nothing — no default fallback, per SPEC 2.3).
- shared-state: snapshot gains `bool allowlist_mode` + `bool mask_all` (separate
  documented semantics per mode).
- frame-decide: new `allowlist_mode` param (filter-configured mode is authoritative);
  mode-transition trigger (snapshot produced under the other mode -> rects untrusted,
  one-tick transient); allowlist failure direction -> `mask_all`; blocklist overflow ->
  chip + keep the rects we have; `struct ss_frame_decision` gains `bool mask_all`.
- filter: `mode` dropdown (blocklist default — v0.1 behavior preserved on upgrade) with
  show/hide modified-callback for the two separate list keys; `allowlist` stored under
  its own settings key; panic hotkey via `obs_hotkey_register_source`
  ("streamsentry.panic", toggle, unregistered on destroy, not persisted);
  `draw_full_plate()` full-source privacy-plate path for panic/mask_all.
- locale: +6 strings (Mode, ModeBlocklist, ModeAllowlist, Allowlist, AllowlistHint,
  PanicHotkey).
- tests: frame-decide +28 CHECKs across 10 M7 cases; watcher-selftest allowlist leg.

## 1. Commands executed (verbatim)
```
rm -rf F:/obsplugin/build_x64
cmake --preset windows-x64-local
cmake --build --preset windows-x64-local
cmake --preset windows-x64-local -DSTREAMSENTRY_PERF_LOG=ON
cmake --build --preset windows-x64-local
cmake --preset windows-x64-local -DSTREAMSENTRY_PERF_LOG=OFF
cmake --build --preset windows-x64-local
ctest -C RelWithDebInfo --output-on-failure          # cwd: F:/obsplugin/build_x64
./coord-map-tests.exe; ./plate-gen-tests.exe; ./toast-gate-tests.exe; ./frame-decide-tests.exe
./watcher-selftest.exe                               # cwd: build_x64/RelWithDebInfo
```

## 2. Build results — PASS; compile/link logs zero-warning; two out-of-tree configure warnings listed below
| Step | Exit | Cache | Warnings/Errors in compile/link log |
|---|---|---|---|
| Clean configure (PERF_LOG default) | 0 (24.9 s) | `STREAMSENTRY_PERF_LOG:BOOL=OFF` | — (see configure note) |
| Build RelWithDebInfo, PERF_LOG=OFF | 0 | OFF | 0 |
| Reconfigure + build, PERF_LOG=ON | 0 | ON | 0 (dll 77,312 B) |
| Restore OFF, reconfigure + build | 0 | OFF — tree left OFF | 0 (dll 74,240 B) |

Warning scan (`grep -c -iE "warning|警告|error C|錯誤"`, zh-TW MSBuild locale — both
English and localized patterns): **0 hits in every build log** (all three builds).

**Configure-log warnings, listed per policy (accepted, out-of-tree, not from this
diff):** each of the three configure runs printed the same two CMake warnings from the
template's OBS-sources dependency bootstrap, i.e. from `.deps/obs-studio-31.1.1`, NOT
from the plugin tree (the plugin repo contains no `FindDetours.cmake`; the staged diff
touches no CMake files):
1. `CMake Warning (dev) at cmake/finders/FindDetours.cmake:65` — "Failed to find
   detours version." (call stack: `plugins/win-capture/graphics-hook/CMakeLists.txt:3`)
2. `CMake Warning at plugins/win-dshow/virtualcam-module/CMakeLists.txt:14` — "Empty
   Virtual Camera GUID set."
Both occur while configuring the vendored OBS sources that provide libobs; they do not
affect the plugin targets, whose own compile/link output is warning-free. (Prior
verifier reports scanned the build logs; noted here because this run also scanned the
configure logs.) The known cosmetic bootstrap typo "Build OBS sources (Reelase - x64)"
appeared again (pre-existing, not a warning).

Build tail (final OFF build) includes:
```
  frame-decide.c
  streamsentry.vcxproj -> F:\obsplugin\build_x64\RelWithDebInfo\streamsentry.dll
  frame-decide-tests.vcxproj -> F:\obsplugin\build_x64\RelWithDebInfo\frame-decide-tests.exe
```

## 3. Test results
### ctest (RelWithDebInfo) — 4/4 suites PASS
```
1/4 Test #1: coord-map-tests ..................   Passed    0.02 sec
2/4 Test #2: plate-gen-tests ..................   Passed    0.14 sec
3/4 Test #3: toast-gate-tests .................   Passed    0.02 sec
4/4 Test #4: frame-decide-tests ...............   Passed    0.02 sec
100% tests passed, 0 tests failed out of 4
```
Direct runs: all four exit 0 ("all passed"). Static CHECK() assertion counts (macro
definition line excluded): coord-map 35, plate-gen 24, toast-gate 23, **frame-decide 53
(was 25 — +28 from the 10 new M7 cases)**; 135 total. The three suites not in the diff
are unchanged. New M7 frame-decide cases (from the staged diff's case comments):
allowlist healthy -> per-window plates; allowlist + stale -> mask-all; allowlist + no
snapshot -> mask-all; mode transition in both directions -> unverified ("mode
transition pending"); allowlist + geometry unresolved -> mask-all; allowlist + INVALID
rect -> mask-all; allowlist + degraded ONLY -> per-window masking stays self-sufficient;
overflow in allowlist -> mask-all (the mode's default); overflow in blocklist -> mask
what we have + chip ("detection overflow (some masks dropped)").

### watcher-selftest.exe — all deterministic checks PASS (exit 0)
Not ctest-registered by design (launches notepad, toggles global watcher mode). Verbatim:
```
ok:   watcher thread started
watcher thread started (tick 150ms)
ok:   heartbeat advances while watcher alive
info: baseline blocklist rects = 0
ok:   launched notepad.exe
ok:   blocklist rect appeared after notepad opened
ok:   blocklist rect disappeared after notepad closed
watcher: matching mode -> allowlist
ok:   allowlist mode masks unapproved windows (rects or mask_all)
watcher: matching mode -> blocklist
ok:   blocklist mode restored after switching back
watcher: DEBUG kill engaged (heartbeat frozen)
ok:   heartbeat frozen while killed (>500ms) -> render shows protection-degraded chip
watcher: DEBUG kill released
ok:   heartbeat resumes after un-kill
INCONCLUSIVE: no toast banner detected (likely Do Not Disturb / focus assist suppressing the banner) - see reports/M2-DECISIONS.md; real toast masking is on the human checklist
watcher thread stopped
ok:   watcher thread stopped cleanly
watcher-selftest: all deterministic checks passed
```
Both NEW M7 legs pass ("allowlist mode masks unapproved windows", "blocklist mode
restored after switching back"). The toast leg is INCONCLUSIVE — same machine-level
banner suppression documented in reports/M6-toast-probe.txt; expected, not a
regression, does not gate the verdict.

## 4. Artifacts — present
`F:\obsplugin\build_x64\RelWithDebInfo\` (the artifact location documented in
TESTING.md; all timestamped this run, 11:23–11:25):
`streamsentry.dll` (74,240 B, final PERF_LOG=OFF build) + `streamsentry.pdb`
(1,773,568 B), `coord-map-tests.exe`, `plate-gen-tests.exe`, `toast-gate-tests.exe`,
`frame-decide-tests.exe`, `watcher-selftest.exe` (+ `w32-pthreads.dll` beside the
self-test per CMake post-build rule). String check on the fresh OFF dll: M7 strings
present ("PANIC engaged by hotkey", "streamsentry.panic"), PERF string
("PERF watcher tick") absent — tree correctly left OFF.

Deployment note (informational per task instruction — noted, not failed): an M7 perf
build is deployed for the owner's hands-on pass at
`D:\software\obs\obs-studio\obs-plugins\64bit\streamsentry.dll` (77,312 B,
2026-07-10 08:28) + pdb (1,781,760 B). Corroboration: byte-size equal to this run's
PERF_LOG=ON build (77,312 B); grep -a hits = 1 each for the M7 strings ("PANIC engaged
by hotkey", "matching mode -> ", "streamsentry.panic") AND for "PERF watcher tick" —
i.e. an M7 PERF_LOG=ON build. Byte-level provenance beyond that is not
machine-verified here.

## 5. Not covered by automation (explicit — these SPEC v0.2 acceptance rows are owner-manual)
1. **In-OBS panic hotkey behavior** (SPEC 2.4 acceptance): engage/release toggling via
   a real OBS hotkey binding; full-source plate visible on the very next rendered frame
   INSTEAD of the target; panic staying engaged during injected watcher death with the
   status chip stacking on top; fresh-session reset (not persisted). The hotkey callback
   and registration compile and the mask_all decision logic is unit-tested, but
   obs_hotkey_register_source needs a live OBS runtime — no automated coverage.
2. **Allowlist mode visual behavior in OBS** (SPEC 2.3 acceptance rows): approved app
   visible while every other window gets a per-window plate; taskbar/desktop plated
   until approved (default-deny first-run consequence); the one-tick mode-switch
   transient; more than SS_MAX_RECTS unapproved windows -> single full-source privacy
   plate (not black); allowlist + watcher killed -> mask-all plate within 500 ms. The
   decision layer for all of these is unit-tested against synthetic snapshots and the
   self-test leg proves live rect/mask_all publication, but rendered pixels are
   owner-manual.
3. **Properties UI show/hide** (mode dropdown swapping the blocklist/allowlist
   textboxes via the modified callback) and the **mode-switch round-trip acceptance
   row** (both lists survive untouched — separate settings keys verified by inspection
   only; obs_data settings need the OBS runtime).
4. **Real toast masking / soak rows** — toast banners are system-suppressed on this
   machine (reports/M6-toast-probe.txt): the self-test toast leg stays INCONCLUSIVE;
   real-toast masking, no-mask-storm soak, and toast-vs-allowlist independence
   (approving explorer.exe must NOT exempt its toasts) are on the human checklist.
5. **Render-path pixels** (src/filter.c graphics calls: full-source plate draw, chip
   drawing, per-rect plates, solid-fill fallback) have no unit harness —
   compile-verified only; only the pure modules (coord-map, plate-gen buffer logic,
   toast-gate, frame-decide) are unit-tested.
6. **Multi-instance semantics** (documented last-writer-wins for the process-global
   watcher mode/lists across multiple filter instances) — documented in watcher.h, not
   exercised by any automated test.

## 6. Verdict
**VERIFIED** — clean from-scratch RelWithDebInfo builds in both PERF_LOG variants with
zero compile/link warnings (tree left at default OFF; the only warnings anywhere were
the two pre-existing out-of-tree OBS-sources configure warnings listed in section 2),
ctest 4/4 PASS with the frame-decide suite grown to 53 asserts covering the 10 M7
decision cases, watcher self-test deterministic legs all PASS including both new
allowlist legs (expected INCONCLUSIVE toast leg only), all artifacts present at the
documented location, owner-pass deployment corroborated as an M7 perf build.

---

# RUN-2 ADDENDUM — post-audit remediation tree (AUTHORITATIVE for the final staged tree)

## What happened
After the run-1 VERIFIED above, the spec-guardian's round-1 audit found violations
requiring code changes (audit record: `reports/M7-spec-guardian.md`, now staged).
Remediation landed; the staged diff grew from 10 files (+470/−70) to 12 files
(+997/−87 — the count now also includes the two staged report files). Run 2 re-executed
the full standard sequence from scratch on 2026-07-10, 11:44–11:49 local.
**This addendum supersedes run 1 and is authoritative.**

## Delta since run 1 (coordinator description, spot-verified via r1-vs-r2 blob diffs)
- `src/watcher.cpp` (`0c06de7` -> `ef8decf`): `EnumWindows` return value now checked —
  on failure the tick is NOT published (skip publish, guardian M7 V1) with
  `g_enum_failed` transition logging (`"watcher: EnumWindows FAILED; tick not
  published - ..."`, never silent). Cloak query is now tri-state
  (`CLOAK_NO/YES/UNKNOWN`) and mode-aware (guardian M7 Q2): blocklist skips an
  UNKNOWN-cloak window (a plate over a not-displayed window would be a wrong mask);
  allowlist falls through and masks it (skipping would punch a confidence-less
  pass-through hole in default-deny).
- `src/filter.c` (`66ba506` -> `9e28980`): the render begin-failure branch is now
  mode-aware (guardian M7 V2) — allowlist mode with masks pending draws the
  full-source plate instead of failing open (`if (f->mode_allowlist) draw_full_plate`);
  panic engage log gains a suffix when the filter is disabled
  (`" (filter currently disabled - takes effect when enabled)"`).
- `src/frame-decide.c/.h` (`1deecd5` -> `e5a2ad0`, `12263a8` -> `81b3d71`): allowlist +
  `detection_degraded` now falls to mask-all (guardian M7 Q1 — an approved toast host
  would otherwise show a real toast during degradation; default-deny must not depend on
  which processes the user approved). Blocklist + degraded keeps the M6.5 V6 guarantee
  (confident masks kept + chip). Header contract text updated to match.
- `tests/frame-decide-tests.c` (`d49d673` -> `4491f88`): the allowlist-degraded case is
  flipped to expect `mask_all`, and a NEW blocklist-degraded companion case pins the V6
  guarantee. Asserts 53 -> 55.
- `ARCHITECTURE.md` (`4567c8b` -> `cac637f`): doc rows updated (doc-only).
- Unchanged blobs from run 1 (confirmed): `CMakeLists.txt` `a3e94f5` (still no CMake
  changes), `src/shared-state.h` `43340e2`, `src/watcher.h` `2ffbd95`,
  `data/locale/en-US.ini` `f4d1e5e`, `tests/watcher-selftest.cpp` `5cba92e`.

Tree identity for this addendum (HEAD still `37d8145`, working tree == index for all
source files): `src/watcher.cpp ef8decf · src/filter.c 9e28980 ·
src/frame-decide.c e5a2ad0 · src/frame-decide.h 81b3d71 ·
tests/frame-decide-tests.c 4491f88 · ARCHITECTURE.md cac637f`.

## Commands executed (verbatim, identical sequence to run 1)
```
rm -rf F:/obsplugin/build_x64
cmake --preset windows-x64-local
cmake --build --preset windows-x64-local
cmake --preset windows-x64-local -DSTREAMSENTRY_PERF_LOG=ON
cmake --build --preset windows-x64-local
cmake --preset windows-x64-local -DSTREAMSENTRY_PERF_LOG=OFF
cmake --build --preset windows-x64-local
ctest -C RelWithDebInfo --output-on-failure          # cwd: F:/obsplugin/build_x64
./coord-map-tests.exe; ./plate-gen-tests.exe; ./toast-gate-tests.exe; ./frame-decide-tests.exe
./watcher-selftest.exe                               # cwd: build_x64/RelWithDebInfo
```

## Run-2 build results — PASS; compile/link logs zero-warning
| Step | Exit | Cache | Warnings/Errors in compile/link log |
|---|---|---|---|
| Clean configure (PERF_LOG default) | 0 (23.3 s) | `STREAMSENTRY_PERF_LOG:BOOL=OFF` | — (see note) |
| Build RelWithDebInfo, PERF_LOG=OFF | 0 | OFF | 0 (dll 74,752 B) |
| Reconfigure + build, PERF_LOG=ON | 0 | ON | 0 (dll 77,312 B) |
| Restore OFF, reconfigure + build | 0 | OFF — tree left OFF | 0 (dll 74,752 B) |

Warning scan (`grep -c -iE "warning|警告|error C|錯誤"`, zh-TW MSBuild locale): **0 hits
in every build log**. Each configure log again contains only the same two out-of-tree
OBS-sources bootstrap warnings already listed in section 2 (FindDetours version /
virtualcam GUID, both from `.deps/obs-studio-31.1.1` — accepted, pre-existing, not from
this diff).

## Run-2 test results — 4/4 suites PASS
```
1/4 Test #1: coord-map-tests ..................   Passed    0.02 sec
2/4 Test #2: plate-gen-tests ..................   Passed    0.15 sec
3/4 Test #3: toast-gate-tests .................   Passed    0.02 sec
4/4 Test #4: frame-decide-tests ...............   Passed    0.02 sec
100% tests passed, 0 tests failed out of 4
```
Direct runs: all four exit 0 ("all passed"). Static CHECK() counts (macro line
excluded): coord-map 35, plate-gen 24, toast-gate 23, **frame-decide 55 (53 -> 55:
flipped allowlist-degraded case + new blocklist-degraded companion)**; 137 total.

watcher-selftest.exe — all deterministic checks PASS (exit 0). Output identical to the
run-1 transcript above, line for line: both M7 allowlist legs pass ("allowlist mode
masks unapproved windows (rects or mask_all)", "blocklist mode restored after switching
back"), kill/recover legs pass, clean stop. The toast leg is again INCONCLUSIVE due to
the machine-level banner suppression (reports/M6-toast-probe.txt) — not a regression,
does not gate the verdict.

## Run-2 artifacts — present
`F:\obsplugin\build_x64\RelWithDebInfo\` (2026-07-10 11:46–11:47):
`streamsentry.dll` (74,752 B, final PERF_LOG=OFF build) + `streamsentry.pdb`
(1,773,568 B), `coord-map-tests.exe`, `plate-gen-tests.exe`, `toast-gate-tests.exe`,
`frame-decide-tests.exe`, `watcher-selftest.exe`, `w32-pthreads.dll`. String check on
the fresh OFF dll: r2 strings present ("EnumWindows FAILED", "takes effect when
enabled", "PANIC engaged by hotkey" — 1 hit each), "PERF watcher tick" absent — tree
correctly left OFF.

Deployment note (informational per coordinator — noted, not failed): the owner-pass DLL
at `D:\software\obs\obs-studio\obs-plugins\64bit\streamsentry.dll` was refreshed to
"M7-r2" (2026-07-10 11:43, 77,312 B; pdb 1,781,760 B). Corroboration: it contains BOTH
r2-distinguishing strings ("EnumWindows FAILED", "takes effect when enabled" — absent
from the run-1 code) AND "PERF watcher tick" (1 hit each), and its byte size equals
this run's PERF_LOG=ON build. Caveat recorded: the run-1 ON build had the same byte
size (77,312 B — PE section rounding), so the identification as r2 rests on the string
evidence, which is conclusive for those literals; byte-level provenance beyond that is
not machine-verified here.

## Addendum to coverage gaps
Run-1 section 5 applies unchanged. Added by this delta:
- IMPROVED: allowlist + detection_degraded -> mask-all and blocklist + degraded ->
  masks-kept + chip are now both pinned by the frame-decide unit suite.
- The EnumWindows-failure skip-publish path and the CLOAK_UNKNOWN mode-aware handling
  live in the Win32 enumeration loop — verified by inspection and compilation only (the
  self-test exercises the loop but cannot force EnumWindows or DwmGetWindowAttribute to
  fail). Stale-heartbeat consequences of repeated enum failures are covered generically
  by the kill-leg, not by an enum-failure-specific automated test.
- The mode-aware begin-failure branch in `src/filter.c` (allowlist -> full-source plate
  when the filter begin call fails) is render-path code with no unit harness —
  compile-verified only; owner-manual.
- The panic-engage-while-disabled log suffix needs a live OBS runtime — owner-manual.

## RUN-2 VERDICT (AUTHORITATIVE, for the run-2 staged tree identified above): VERIFIED
Clean from-scratch zero-warning RelWithDebInfo builds in both PERF_LOG variants (tree
left at default OFF; only the two known out-of-tree configure warnings), ctest 4/4 PASS
with the guardian-driven test flips in place (frame-decide 55 asserts), watcher
self-test deterministic legs all PASS including both allowlist legs (expected
INCONCLUSIVE toast leg only), all artifacts present, owner-pass deployment corroborated
as an M7-r2 PERF_LOG=ON build.
