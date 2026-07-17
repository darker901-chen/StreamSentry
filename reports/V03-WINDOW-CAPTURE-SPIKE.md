# v0.3 Window Capture geometry spike

**Status:** implementation spike; Windows/OBS build and live acceptance are **not yet verified**.

## Goal

Allow StreamSentry to map watcher screen-space rectangles onto an OBS Windows
**Window Capture** source without guessing the captured HWND or its origin.
Display Capture behavior must remain unchanged.

## Confidence contract

The resolver returns a usable geometry only when all of these are true:

1. The target source id is `window_capture`.
2. OBS's source-local `get_hooked` procedure reports an active capture and the
   current title, class, and executable.
3. That exact identity maps to **one and only one** current top-level HWND.
4. The HWND exposes a non-empty client rectangle and/or full-frame rectangle.
5. Exactly one unique candidate rectangle has dimensions identical to the OBS
   source base dimensions.

Any missing API, unhooked/minimized source, duplicate identity, dimension
mismatch, or ambiguous rectangle returns failure. Existing frame-decision
semantics then show the protection-degraded chip in blocklist mode or the
allowlist mask-all fallback. The spike never chooses a best-effort rectangle.

## Implementation notes

- Uses OBS's public proc-handler surface instead of parsing the private
  serialized `window` setting or copying OBS's internal match-priority logic.
- Uses the active hooked identity to enumerate Windows HWNDs and compares the
  exact UTF-8 title/class plus case-insensitive executable basename.
- Considers three Windows-authoritative geometry candidates: client area in
  screen coordinates, DWM extended frame bounds, and `GetWindowRect`.
- Deduplicates identical rectangles and accepts only one exact size match.
- Keeps the existing unambiguous monitor-size path unchanged.

## Checks performed here

- C11 structural syntax check with minimal OBS/Win32 API stubs: **passed**.
- Real Visual Studio + OBS SDK build: **not run** (Windows toolchain unavailable
  in this execution environment).
- OBS runtime and performance measurements: **not run**.

The stub syntax check is not release evidence and must not be copied into a
verifier report as though it were an OBS build.

## Required Windows verification

### Build gate

1. Configure and build the normal Windows x64 preset with no new warnings.
2. Run all existing CTest suites; all must remain green.
3. Confirm the DLL imports only the already-approved OBS/Windows dependencies.

### Display Capture regression

1. Repeat the existing single-monitor Display Capture masking smoke test.
2. Verify ambiguous identical-resolution monitors still produce degraded rather
   than a guessed origin.

### Window Capture acceptance

Run each case with a password field or blocklisted target that creates a pending
mask rectangle:

1. BitBlt/legacy Window Capture client area: mask lands on the correct region.
2. WGC Window Capture with **Client Area** enabled: mask lands correctly.
3. WGC Window Capture with **Client Area** disabled: full-frame coordinates land
   correctly, including the title-bar offset.
4. Move and resize the captured window across monitors with different DPI: the
   mask follows after the next watcher update without a wrong-position frame.
5. Minimize or close the captured window: protection becomes degraded/mask-all;
   no stale mask remains at the old position.
6. Open two windows with the same title/class/executable identity: resolver must
   reject the ambiguity and show degraded/mask-all instead of choosing either.
7. Change a dynamic window title while captured: temporary mismatch may degrade,
   but must never place a mask on another window.

### Performance gate

Measure render-decision cost and watcher load in allowlist mode with many open
windows. The current spike enumerates top-level windows while masks are pending;
if this exceeds the existing performance budget, cache only at snapshot
heartbeat boundaries and re-run the ambiguity tests before considering merge.

## Merge boundary

Do not merge or advertise Window Capture support until the repository's normal
verifier and spec-guardian gates produce VERIFIED/PASS evidence and the human
OBS checks above are recorded in `TESTING.md`.
