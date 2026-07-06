# ARCHITECTURE.md — StreamSentry as built (v0.1)

This documents the plugin **as actually built and shipped in v0.1**
(HEAD at M4), not aspirations. When code and this file disagree, the
code is the bug or this file is — fix one. The governing constraints
live in CLAUDE.md (iron rules) and SPEC.md; this file explains how the
implementation satisfies them.

## Module map

| Module | Language / deps | Role |
|---|---|---|
| [src/watcher.cpp](src/watcher.cpp) (+ watcher.h C interface) | C++, Win32 + COM/UIA + dwmapi + psapi. No libobs link dependency (obs_log/os_gettime_ns declared plainly, resolved by import-lib thunk in the plugin and by stubs in the selftest) | The one detection thread. Win32 window enumeration on a 150ms timer (toasts + blocklist), UIA focus-changed events (password fields). Publishes screen-space rects + heartbeat. |
| [src/shared-state.c](src/shared-state.c) / .h | C, libobs `util/threading` (pthread mutex) | The only data channel between watcher and render: a fixed-size snapshot (`SS_MAX_RECTS = 64` rects + heartbeat) behind one mutex. Writer blocks briefly; reader is trylock-only. |
| [src/coord-map.c](src/coord-map.c) / .h | Pure C, no OBS, no Windows | Screen-rect → source-rect mapping: intersect with captured region, scale, pad (over-mask), clamp. Unit-tested standalone. Any degenerate/non-finite input → `SS_MAP_INVALID`, which callers must treat as fail-closed. |
| [src/geom-resolve.c](src/geom-resolve.c) / .h | C, Win32 + libobs | Resolves *what the filter's target source captures* (virtual-screen region + source pixel size). v0.1: `monitor_capture` only, and only when exactly one monitor's pixel size matches the source base size. Everything else → false → caller fails closed. |
| [src/plate-gen.c](src/plate-gen.c) / .h | Pure C, no OBS, no Windows | CPU generator for mask visuals (RGBA8888): toast card, privacy plate, fail-closed status banner. Opacity (iron rule 3) is a *tested property*: `ss_image_opaque_inside()` + `ss_plate_max_corner_inset()` let unit tests prove every pixel inside the corner inset is alpha-255. |
| [src/filter.c](src/filter.c) / filter.h | C, libobs | The OBS video filter: settings, per-frame health decision, coordinate mapping, plate texture cache, fail-closed rendering. |
| [src/plugin-main.c](src/plugin-main.c) | C, libobs | Module entry: `ss_state_init()` + `obs_register_source(&streamsentry_filter_info)`. |
| src/plugin-support.c.in | template | obs-plugintemplate logging support (`obs_log`). |

Dependency direction: `filter.c` → {coord-map, plate-gen, shared-state,
geom-resolve, watcher-interface}. `watcher.cpp` → shared-state only.
The two pure modules (coord-map, plate-gen) import nothing platform-
specific — that is what makes the security properties unit-testable
(ctest) without OBS or a display.

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
   fails closed by "no detection snapshot".
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
| `g_cfg_mutex` (watcher) | blocklist vector | `ss_watcher_set_blocklist` (settings thread), watcher tick **for the whole EnumWindows pass** (so a settings update never observes a half-applied list) |
| `g_pw_mutex` (watcher) | password-field rect | UIA callbacks, watcher tick |
| `state_mutex` (shared-state) | the one snapshot | watcher publish (blocking, brief memcpy), render trylock |

Notes as built: the blocklist is process-global because the watcher is
shared — with multiple StreamSentry filter instances, the most recently
updated instance's blocklist wins (documented in filter.c).
`ss_state_touch_heartbeat()` exists in the API but has **no callers**:
the heartbeat is only ever written together with a full successful
publish, so a heartbeat always certifies a *completed* detection pass,
never a partially-alive loop. Keep it that way.

## Data flow (one healthy frame)

```
watcher thread, every 150ms                     OBS graphics thread, every frame
---------------------------                     --------------------------------
EnumWindows pass:                               filter_video_render:
  visible? not DWM-cloaked? non-empty rect?       enabled? target has size? else skip
  toast:  proc==explorer.exe                      ss_state_try_read -> snapshot
          AND class==Xaml_WindowedPopupClass      heartbeat age > 500ms? -> FAIL-CLOSED
  block:  entry substring-matches process         rects present?
          image name OR window title                ss_resolve_capture_geom(target)
UIA focus rect (if valid) appended                   (monitor_capture, unambiguous
snapshot.heartbeat = os_gettime_ns()                  monitor match only) else FAIL-CLOSED
ss_state_publish(snapshot)                           ss_map_screen_rect(+12px pad) each rect
                                                     OK -> keep; NOT_VISIBLE -> skip;
                                                     INVALID -> FAIL-CLOSED
                                                  render target through filter chain
                                                  draw plate texture per mapped rect
                                                  (toast card / privacy plate)
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

## Fail-closed state machine (render side)

States: `INIT` → `NORMAL` ⇄ `FAIL_CLOSED` (per filter instance; used
for transition logging, not behavior — behavior is recomputed every
frame from scratch, so there is no state to get stuck in).

A frame renders **full black + status banner** ("Privacy guard:
detection unavailable — output blocked") when any of these hold, in
evaluation order:

| # | Trigger (log reason) | Source |
|---|---|---|
| 1 | `no detection snapshot` — never read a snapshot since filter create | filter.c |
| 2 | `heartbeat stale` — snapshot heartbeat older than `SS_HEARTBEAT_STALE_NS` = 500ms (also fires if heartbeat is in the future) | filter.c |
| 3 | `capture geometry unresolved` — rects pending but target is not monitor_capture, has zero size, or no/ambiguous monitor match (incl. two identical-resolution monitors) | geom-resolve.c |
| 4 | `coordinate mapping failed` — any rect maps to `SS_MAP_INVALID` | coord-map.c |
| 5 | `filter chain bypassed` — `obs_source_process_filter_begin` returned false | filter.c |
| 6 | `plate texture allocation failed` — plate generation or `gs_texture_create` failed mid-draw (black is drawn *over* the already-rendered target) | filter.c |

Ordering that matters, as built: **all rects are mapped before the
target is rendered** (triggers 3–4 are evaluated before
`process_filter_begin`), so a mapping failure blacks the frame rather
than leaving a partially-masked target on screen. Trigger 6 is the one
mid-draw case; it paints black over whatever was already composed.

Deliberate non-triggers: `enabled == false` (explicit user off-switch —
the whole feature is off, SPEC settings UI) and a zero-sized target
both `obs_source_skip_video_filter()`. An *empty* rect list with a
healthy heartbeat is pass-through — geometry is only resolved when
there is something to mask, so an unsupported source type stays usable
until the first detection, at which point it fails closed (documented
v0.1 limitation).

Watcher-side guarantees feeding this: the heartbeat is written only at
the end of a fully successful enumerate-and-publish tick, and the
fault-injection kill (`ss_watcher_debug_set_killed`, selftest-only)
freezes publish *and* heartbeat exactly like a dead thread. Fail-closed
engage/clear transitions are logged once each (no per-frame spam), with
the heartbeat age in ms.

## Render-side caching (why per-frame cost is flat)

- **Plate texture cache**: 8-entry LRU keyed by (style, bucketed size).
  `bucket_dim()` rounds sizes up to 16px buckets (min 32), so a
  drifting toast doesn't regenerate textures every frame; the drawn
  sprite is stretched from bucket size to the exact mapped rect.
  Styles collapse to two (toast card / privacy plate) since FIELD and
  WINDOW share the plate.
- **Status banner**: generated once per filter instance, cached until
  destroy; centered and width-scaled at draw time (skipped below
  160×60 target size — the black fill alone carries the guarantee).
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
| `watcher-selftest` (manual exe, not registered with `add_test`) | integration | real EnumWindows/UIA against a live desktop: spawns notepad, fires a toast, exercises blocklist matching and the kill→stale-heartbeat path with obs stubs |

The selftest links `watcher.cpp + shared-state.c` directly with its own
`obs_log`/`os_gettime_ns`/pthread stubs — that is why watcher.cpp must
not include dllimport-decorated OBS headers.

## Known v0.1 architectural limits (documented, not bugs)

- Only `monitor_capture` sources resolve geometry; window/game capture
  fails closed on first detection. (geom-resolve.c)
- Two monitors with identical pixel size are indistinguishable →
  ambiguous → fail closed. (geom-resolve.c)
- Toast signature is per-Windows-build-family (verified on Win11 26200:
  explorer.exe + Xaml_WindowedPopupClass, which over-matches other
  explorer XAML flyouts — over-mask by design; v0.2 narrows this by
  geometry). SPEC.md's Win10-era ShellExperienceHost example is
  superseded by the code comment. (watcher.cpp)
- Blocklist matching is case-insensitive substring against process
  image name **OR** window title — owner ruling FINAL, commit a434b18.
- Per-tick `OpenProcess`/`QueryFullProcessImageNameW` for every visible
  window is the suspected cause of rare heartbeat-stall blips under
  streaming load (v0.2 M6 target: PID→name cache + tick p99
  measurement).
- `SS_MAX_RECTS = 64` caps reported rects; the enum pass stops at the
  cap (v0.1 never hit it in the field — 27 was the observed max).
- Chromium password fields: a11y tree not always active → focus events
  may not fire (documented limitation; v0.2 investigation item).
