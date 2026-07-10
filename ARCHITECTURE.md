# ARCHITECTURE.md — StreamSentry as built (v0.1 + v0.2 through M7)

This documents the plugin **as actually built**, not aspirations —
v0.1 as shipped at M4, the M6 hardening changes (marked "M6"), and the
M6.5 fail-open repositioning (owner ruling 2026-07-09,
reports/RULING-2026-07-09-fail-open.md: never disrupt the output; mask
only on confidence; the full-frame blackout is removed). When code and
this file disagree, the code is the bug or this file is — fix one. The
governing constraints live in CLAUDE.md (iron rules, as amended
2026-07-09) and SPEC.md (§2.7); this file explains how the
implementation satisfies them.

## Module map

| Module | Language / deps | Role |
|---|---|---|
| [src/watcher.cpp](src/watcher.cpp) (+ watcher.h C interface) | C++, Win32 + COM/UIA + dwmapi + psapi. No libobs link dependency (obs_log/os_gettime_ns declared plainly, resolved by import-lib thunk in the plugin and by stubs in the selftest) | The one detection thread. Win32 window enumeration on a 150ms timer (toasts + blocklist), UIA focus-changed events (password fields). Publishes screen-space rects + heartbeat. |
| [src/shared-state.c](src/shared-state.c) / .h | C, libobs `util/threading` (pthread mutex) | The only data channel between watcher and render: a fixed-size snapshot (`SS_MAX_RECTS = 64` rects + heartbeat) behind one mutex. Writer blocks briefly; reader is trylock-only. |
| [src/coord-map.c](src/coord-map.c) / .h | Pure C, no OBS, no Windows | Screen-rect → source-rect mapping: intersect with captured region, scale, pad, clamp. Unit-tested standalone. Any degenerate/non-finite input → `SS_MAP_INVALID`: that detection gets no mask and the frame is flagged DEGRADED (chip) — M6.5, SPEC 2.7. |
| [src/geom-resolve.c](src/geom-resolve.c) / .h | C, Win32 + libobs | Resolves *what the filter's target source captures* (virtual-screen region + source pixel size). v0.1: `monitor_capture` only, and only when exactly one monitor's pixel size matches the source base size. Everything else → false → caller renders DEGRADED (chip) while masks are pending. |
| [src/plate-gen.c](src/plate-gen.c) / .h | Pure C, no OBS, no Windows | CPU generator for mask visuals (RGBA8888): toast card, privacy plate, failure status chip. Opacity (iron rule 3) is a *tested property*: `ss_image_opaque_inside()` + `ss_plate_max_corner_inset()` let unit tests prove every pixel inside the corner inset is alpha-255. |
| [src/toast-gate.c](src/toast-gate.c) / .h (M6) | Pure C, no OBS, no Windows | Toast geometry gate (SPEC 2.2): right-edge spawn band + generous size envelope, evaluated per monitor. Applied *after* the process+class signature; only removes toast-card over-masking. M6.5: uncertainty (no monitors, degenerate input) classifies as NOT a toast — mask only on confidence (SPEC 2.7). Constants are PROVISIONAL documented-metrics values (reports/M6-toast-probe.txt) pending on-machine calibration. |
| [src/frame-decide.c](src/frame-decide.c) / .h (M6.5, M7) | Pure C, no OBS, no Windows | The per-frame masking decision (SPEC 2.7 ordering): health → mode-transition → degraded-flag → overflow → geometry → per-rect mapping; guarantees a degradation flag never drops a blocklist confident mask (guardian V6 regression is a unit test). M7: allowlist failure direction — ANY unverified state yields `mask_all` (the mode's default-deny, guardian Q1); blocklist overflow masks what it has + chip. filter.c only resolves geometry, calls this, and draws. |
| [src/filter.c](src/filter.c) / filter.h | C, libobs | The OBS video filter: settings (mode + both lists, M7), panic hotkey (M7, full-source plate instead of the target, not persisted), plate texture cache, DEGRADED-chip and mask-all rendering. |
| [src/plugin-main.c](src/plugin-main.c) | C, libobs | Module entry: `ss_state_init()` + `obs_register_source(&streamsentry_filter_info)`. |
| src/plugin-support.c.in | template | obs-plugintemplate logging support (`obs_log`). |

Dependency direction: `filter.c` → {frame-decide, coord-map, plate-gen,
shared-state, geom-resolve, watcher-interface}. `watcher.cpp` →
{shared-state, toast-gate}. The four pure modules (coord-map,
plate-gen, toast-gate, frame-decide) import nothing platform-specific —
that is what makes the security properties unit-testable (ctest)
without OBS or a display.

## Thread model

Three kinds of threads touch this code:

1. **OBS graphics thread** — runs `filter_video_render()` every frame.
   May *never* block: reads shared state with `ss_state_try_read()`
   (trylock; on contention it keeps last snapshot — staleness is still
   governed by the heartbeat check, so a lost read cannot extend trust).
2. **Watcher thread** (one per process, refcounted by filter instances:
   first `ss_watcher_start()` creates it, last `ss_watcher_stop()` sets
   the stop event and joins with a 3s timeout). `CoInitializeEx(MTA)`,
   then a `WaitForSingleObject(stop_event, 150ms)` loop; each tick runs
   `EnumWindows` and publishes one snapshot. If `CreateThread` fails the
   refcount is rolled back and nothing ever publishes → render side
   goes DEGRADED (chip) with "no detection snapshot".
3. **UIA callback threads** — with an MTA client, UI Automation invokes
   `FocusHandler::HandleFocusChangedEvent` on COM worker threads, *not*
   on the watcher thread. The handler does minimal work (iron
   architecture): read `IsPassword`, copy `BoundingRectangle` into
   `g_pw_rect` under `g_pw_mutex`, return. The watcher tick folds that
   rect into the next snapshot. If UIA init/registration fails, the
   plugin logs a warning and runs without the password-field guard
   (toasts/blocklist unaffected) — this degrades a *secondary* guard,
   and the heartbeat still certifies the primary detection loop.

Locks (all short-held):

| Lock | Guards | Held by |
|---|---|---|
| `g_mutex` (watcher) | start/stop refcount + thread handles | filter create/destroy, `ss_watcher_is_running` |
| `g_cfg_mutex` (watcher) | blocklist + allowlist vectors + mode (M7) | `ss_watcher_set_blocklist/allowlist/mode_allowlist` (settings thread), watcher tick **for the whole EnumWindows pass** (so a settings update never observes a half-applied list) |
| `g_pw_mutex` (watcher) | password-field rect | UIA callbacks, watcher tick |
| `state_mutex` (shared-state) | the one snapshot | watcher publish (blocking, brief memcpy), render trylock |

Notes as built: the blocklist is process-global because the watcher is
shared — with multiple StreamSentry filter instances, the most recently
updated instance's blocklist wins (documented in filter.c).
`ss_state_touch_heartbeat()` exists in the API but has **no callers**:
the heartbeat is only ever written together with a full successful
publish, so a heartbeat always certifies a *completed* detection pass,
never a partially-alive loop. Keep it that way.

M6 additions on the watcher thread (no new locks): a PID→image-name
cache (watcher-thread-only; entries evicted after one tick of absence;
failed lookups deliberately not cached; PID-reuse residual race
documented in watcher.cpp per SPEC 2.1), per-tick monitor-rect
enumeration feeding the toast geometry gate, and an always-compiled
LOG_WARNING when a single tick exceeds 250ms (half the stale
threshold). Thread priority is deliberately untouched — SPEC 2.1 makes
it the second lever, only if p99 under load still approaches the
threshold after the cache.

M7 watcher-side guarantees: a genuine `EnumWindows` FAILURE (FALSE
without the overflow flag) skips the publish entirely — the heartbeat
only certifies a completed pass, so the render side goes unverified
within 500ms instead of trusting a partial rect list (guardian M7 V1;
transition-logged). DWM cloak-query failure is mode-aware: blocklist
skips the window (no mask on doubt), allowlist keeps processing it
(default-deny must not leak on doubt — guardian M7 Q2).

## Data flow (one healthy frame)

```
watcher thread, every 150ms                     OBS graphics thread, every frame
---------------------------                     --------------------------------
EnumDisplayMonitors -> monitor rects (M6)       filter_video_render:
EnumWindows pass:                                 enabled? target has size? else skip
  visible? not DWM-cloaked? non-empty rect?       ss_state_try_read -> snapshot
  toast:  proc==explorer.exe                      heartbeat age > 500ms? -> UNVERIFIED
          AND class==Xaml_WindowedPopupClass      rects present?
          AND geometry gate (M6; gate-fail          ss_resolve_capture_geom(target)
          falls through to block/allowlist)          (monitor_capture, unambiguous
  block:  entry substring-matches process             monitor match only) else UNVERIFIED
          image name OR window title                 ss_map_screen_rect(+12px pad) each rect
  allow (M7): NOT-matching windows masked            OK -> keep; NOT_VISIBLE -> skip;
  (proc names via PID cache, M6)                     INVALID -> UNVERIFIED (keep OK rects)
UIA focus rect (if valid) appended                 (health/mode/degraded/overflow/geometry/
snapshot.detection_degraded (M6.5)                  mapping order = pure frame-decide)
snapshot.allowlist_mode + mask_all (M7)           panic or mask_all -> full-source plate
snapshot.heartbeat = os_gettime_ns()                INSTEAD of the target (M7)
ss_state_publish(snapshot)                        else ALWAYS render target through chain,
tick > 250ms -> LOG_WARNING (M6)                  draw plate per confidently mapped rect
                                                  UNVERIFIED -> small status chip on top
                                                  (M6.5: blackout removed, SPEC 2.7)
```

Rect kinds: `SS_RECT_TOAST` → notification card; `SS_RECT_WINDOW` and
`SS_RECT_FIELD` → privacy plate. All watcher rects are **virtual-screen
physical pixels**; the watcher cannot know which source the filter sits
on, so all capture-geometry knowledge is filter-side (geom-resolve),
resolved fresh each frame that has rects to map.

Coordinate chain (as built): screen rect ∩ captured region → scale by
(src_size / region_size) per axis (this is what absorbs mixed-DPI and
scaled captures) → expand by `SS_MASK_PAD_PX = 12` source pixels on
every side (over-mask; must stay above `ss_plate_max_corner_inset()`)
→ clamp to source bounds. Scene-item transforms (scale/crop in OBS) are
applied by OBS *downstream* of the filter, so a rect correct in source
space stays glued under those transforms.

## Failure semantics (render side) — M6.5, SPEC 2.7

States: `INIT` → `NORMAL` ⇄ `DEGRADED` (per filter instance; used for
transition logging, not behavior — behavior is recomputed every frame
from scratch, so there is no state to get stuck in).

Owner ruling 2026-07-09: the output is **never disrupted**. The source
renders through the filter chain on every frame. Confidently detected
AND confidently mapped threats get opaque masks; anything the plugin
cannot verify makes the frame **DEGRADED**: rendered normally with a
small opaque **status chip** ("StreamSentry: protection degraded — see
log") at the top-left (12px margin, ≤ 40% frame width, skipped below
160×60), plus one LOG_WARNING per engagement. The v0.1 full-frame
blackout no longer exists.

DEGRADED triggers, in evaluation order:

| # | Trigger (log reason) | Source | Masks this frame |
|---|---|---|---|
| 1 | `no detection snapshot` — never read a snapshot since filter create | filter.c | none |
| 2 | `heartbeat stale` — heartbeat older than `SS_HEARTBEAT_STALE_NS` = 500ms (also if in the future) | filter.c | none |
| 3 | `mode transition pending` (M7) — the snapshot was produced under the other matching mode; its rects mean the opposite thing (one-tick transient after a mode switch) | frame-decide.c | none |
| 4 | `detection degraded (monitor data unavailable)` — watcher published `detection_degraded` (monitor enumeration failed/truncated → toast gate cannot affirm; heartbeat still fresh; watcher logs the transition) | watcher.cpp → shared-state | blocklist: window+password masks still active; allowlist: mask_all (Q1) |
| 5 | `detection overflow (some masks dropped)` (M7, blocklist mode only) — the enum pass hit the `SS_MAX_RECTS` budget | watcher.cpp → shared-state | the 64 rects we do have still masked |
| 6 | `capture geometry unresolved` — rects pending but target is not monitor_capture, has zero size, or no/ambiguous monitor match (incl. two identical-resolution monitors) | geom-resolve.c | none |
| 7 | `coordinate mapping failed for a detection` — some rect maps `SS_MAP_INVALID` | coord-map.c | **confidently mapped rects still masked** |
| 8 | `filter chain bypassed with masks pending` — `process_filter_begin` failed while masks were due; blocklist: source shown via `skip_video_filter` + chip; allowlist: full-source plate (needs no chain; reachable only with the frame otherwise healthy — unverified allowlist frames already went mask-all before the chain, so the chip line in that arm is defensive dead code) | filter.c | blocklist: none drawable; allowlist: full plate |

Allowlist failure direction (M7, SPEC 2.3): whenever the frame is
unverified for ANY reason — including degraded-only (guardian M7 Q1:
an approved toast host would otherwise show a real toast during
degradation) — allowlist mode sets `mask_all`: one full-source privacy
plate is drawn INSTEAD of the target, chip on top. Watcher rect-budget
overflow in allowlist mode is `mask_all` without the chip — that is
the mode's default state, not a failure.

Not a DEGRADED trigger: plate-texture allocation failure for a
confidently mapped rect draws a **solid opaque fallback fill** instead
(a confident mask is never dropped — iron rule 3 as amended) with a
log warning.

Deliberate non-triggers: `enabled == false` and a zero-sized target
both `obs_source_skip_video_filter()` with no chip (explicit user
off-switch / nothing to protect). An *empty* rect list with a healthy
heartbeat is clean pass-through, chip-free — geometry is resolved only
when something needs masking, so e.g. a window-capture source shows a
chip only while detections exist that cannot be placed.

Watcher-side guarantees feeding this: the heartbeat is written only at
the end of a fully successful enumerate-and-publish tick, and the
fault-injection kill (`ss_watcher_debug_set_killed`, selftest-only)
freezes publish *and* heartbeat exactly like a dead thread. DEGRADED
engage/clear transitions are logged once each (no per-frame spam) with
the heartbeat age in ms.

Panic hotkey (M7, SPEC 2.4): a deliberate user action that outranks
health state — while engaged, the full-source privacy plate is drawn
INSTEAD of the target on every frame (the target is never composed),
with the chip stacked on top whenever protection is simultaneously
unverified. Toggled from the OBS hotkey thread via an atomic bool; not
persisted across sessions; engage/release are logged.

## Render-side caching (why per-frame cost is flat)

- **Plate texture cache**: 8-entry LRU keyed by (style, bucketed size).
  `bucket_dim()` rounds sizes up to 16px buckets (min 32), so a
  drifting toast doesn't regenerate textures every frame; the drawn
  sprite is stretched from bucket size to the exact mapped rect.
  Styles collapse to two (toast card / privacy plate) since FIELD and
  WINDOW share the plate.
- **Status chip**: generated once per filter instance, cached until
  destroy; drawn top-left, width-capped at 40% of the frame (skipped
  below 160×60 target size — the OBS log still carries the warning).
- CPU plate generation exists only on cache miss; steady state is a
  texture lookup + `gs_draw_sprite`.

Measured (M3, reports/M3-perf.md): watcher ≈ 0.55% of one core at
150ms cadence; render decision ≈ 0.03 µs/frame; 30-min soak flat at
~321 MB working set. Perf sampling code is compiled in only with the
`STREAMSENTRY_PERF_LOG` CMake option (off in release).

## Test infrastructure

| Target | Kind | What it proves |
|---|---|---|
| `coord-map-tests` (ctest) | pure unit | mapping math: intersection, mixed-DPI scaling, padding, clamping, INVALID on degenerate input |
| `plate-gen-tests` (ctest) | pure unit | **opacity as a property**: plates opaque inside corner inset; corner inset < mask pad; banner opaque |
| `toast-gate-tests` (ctest, M6) | pure unit | geometry gate: documented toast shapes pass at 100–200% DPI (incl. slide-in and secondary-monitor cases); recorded flyover shapes rejected; **uncertainty classifies as NOT a toast** (no monitors / degenerate / NaN input — M6.5, SPEC 2.7) |
| `frame-decide-tests` (ctest, M6.5, M7) | pure unit | SPEC 2.7 ordering: **degraded flag keeps blocklist confident masks** (guardian V6 regression), stale drops all masks, geometry/mapping failures flag the frame while confident rects stay masked, reason precedence; M7: allowlist fails to mask-all for EVERY unverified reason incl. degraded (guardian Q1), overflow semantics per mode |
| `watcher-selftest` (manual exe, not registered with `add_test`) | integration | real EnumWindows/UIA against a live desktop: spawns notepad, fires a toast, exercises blocklist matching, the M7 allowlist mode switch (unapproved windows masked, restore on switch-back), and the kill→stale-heartbeat path with obs stubs |

The selftest links `watcher.cpp + shared-state.c` directly with its own
`obs_log`/`os_gettime_ns`/pthread stubs — that is why watcher.cpp must
not include dllimport-decorated OBS headers.

## Known v0.1 architectural limits (documented, not bugs)

- Only `monitor_capture` sources resolve geometry; window/game capture
  cannot place masks — since M6.5 that renders normally with the
  protection-degraded chip while detections exist (was: blackout).
  (geom-resolve.c)
- Two monitors with identical pixel size are indistinguishable →
  ambiguous → same chip-degraded behavior. (geom-resolve.c)
- Toast signature is per-Windows-build-family (verified on Win11 26200:
  explorer.exe + Xaml_WindowedPopupClass; over-matches other explorer
  XAML flyouts). SPEC.md's Win10-era ShellExperienceHost example is
  superseded by the code comment. (watcher.cpp) **M6 update:** matches
  are now narrowed by the toast-gate geometry check with PROVISIONAL
  documented-metrics constants — banners were system-suppressed on the
  dev machine 2026-07-09 so live calibration and signature
  re-verification on build 26200.8655 are deferred to the owner's
  acceptance pass (owner ruling; evidence and the shell-class changes
  observed on .8655 in reports/M6-toast-probe.txt).
- Blocklist matching is case-insensitive substring against process
  image name **OR** window title — owner ruling FINAL, commit a434b18.
- ~~Per-tick `OpenProcess`/`QueryFullProcessImageNameW` for every
  visible window~~ **fixed in M6** by the PID→name cache (bounded
  residual: PID reuse across a one-tick gap can serve a stale name for
  proc-name matching; title matching unaffected — see watcher.cpp).
  Tick p99 measurement under streaming load is on the owner's soak
  checklist.
- `SS_MAX_RECTS = 64` caps reported rects; the enum pass stops at the
  cap (v0.1 never hit it in the field — 27 was the observed max).
- Chromium password fields: a11y tree not always active → focus events
  may not fire (documented limitation; v0.2 investigation item).

## Development workflow (multi-agent, gate-per-milestone)

Every milestone in this repo is produced by a hub-and-spoke agent
workflow (owner ruling 2026-07-09: document it here); the spokes talk
to each other only through files in `reports/`:

| Role | Writes | Job |
|---|---|---|
| main session (hub) | src/, tests/, docs | plans and implements the milestone; the only role that edits code |
| **verifier** | `reports/M<n>-verifier.md` | clean from-scratch configure+build, runs every automated test, checks artifacts; never fixes anything; verdict VERIFIED/FAILED |
| **spec-guardian** | `reports/M<n>-spec-guardian.md` | audits the staged diff line-by-line against CLAUDE.md + SPEC.md; no code write access; verdict PASS/FAIL blocks the milestone |
| **scribe** | TESTING.md, CHANGELOG.md | records only what the two reports prove; refuses if either gate is missing or failed |

A milestone lands only as VERIFIED + PASS + scribe record + owner's
manual acceptance items (tracked in TESTING.md). Owner rulings that
amend the rules are recorded as `reports/RULING-*.md` (or in commit
messages, e.g. a434b18) and bind all later audits. The agents are
defined in `.claude/agents/`; evidence is committed verbatim and never
edited after the fact.
