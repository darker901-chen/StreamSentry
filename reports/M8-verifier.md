# M8 Verifier Report — window picker + docs/package refresh + v0.2 FINAL (SPEC §2.5)

- Run: 2026-07-10, 16:55–17:05 local, local machine (Windows 11 Home 10.0.26200)
- Tree under test: HEAD `706cda1` + staged M8 diff (9 files, +348/−26), 0 unstaged changes
- Toolchain: CMake 3.28.0-rc5, preset `windows-x64-local` (generator "Visual Studio 17 2022"
  x64, SDK 10.0.26100.0, MSVC 19.44.35224.0), config RelWithDebInfo, dependency cache
  `.deps` present (obs-studio-31.1.1)
- **Verdict: VERIFIED**

## 0. Scope of the staged diff (verified present)
Staged files (all 9, working tree == index; `CMakeLists.txt` blob `a3e94f5` unchanged
from M6.5/M7 — "no CMake changes" confirmed):
`src/watcher.cpp` `98f15b2` · `src/watcher.h` `a2f1999` · `src/filter.c` `0a0b064` ·
`data/locale/en-US.ini` `58093ff` · `tests/watcher-selftest.cpp` `621b1e1` ·
`buildspec.json` `0554adb` · `README.md` `e4262cc` · `ARCHITECTURE.md` `e022156` ·
`reports/V02-FINAL.md` `600a57c` (new).

Spot-checks against the coordinator's description (all found in the staged diff):
- watcher.h/.cpp: `struct ss_open_window` (proc[64], title[128]) +
  `ss_enum_open_windows(out, max_count)` — one-shot `EnumWindows` on the calling (UI)
  thread; same gates as detection (`IsWindowVisible`, `cloak_state(hwnd) != CLOAK_NO`,
  non-degenerate `GetWindowRect`); plain `proc_image_name` (PID cache stays
  watcher-thread-only); dedupe by process name with title upgrade for entries that had
  none. Detection paths untouched.
- filter.c: `picker_window` combo (`OBS_COMBO_FORMAT_STRING`, label "proc - title",
  value = proc) pre-filled via `picker_fill_combo()` in `filter_get_properties`;
  `picker_refresh` button re-enumerates; `picker_add` button (`obs_properties_add_button2`
  with filter data) appends the selected process to the ACTIVE mode list key
  ("allowlist" iff mode=="allowlist", else "blocklist"), case-insensitive trimmed
  line-dedupe via `list_has_line()`, applied through `obs_source_update`.
- locale: +3 strings (PickerWindow, PickerRefresh, PickerAdd) — keys match filter.c usage.
- tests: watcher-selftest new M8 leg — every enumerated entry must carry a process name,
  no duplicate proc entries.
- buildspec.json: version 0.1.0 -> 0.2.0; README refreshed to v0.2 (mode/picker/panic
  usage, roadmap, `streamsentry-0.2.0-windows-x64.zip`); ARCHITECTURE.md "through M8"
  (+ picker rows); reports/V02-FINAL.md present.

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
rm -rf F:/obsplugin/release
cmake --install F:/obsplugin/build_x64 --prefix F:\obsplugin\release\RelWithDebInfo --config RelWithDebInfo
powershell Compress-Archive -Path streamsentry -DestinationPath F:\obsplugin\release\streamsentry-0.2.0-windows-x64.zip -Force   # cwd: release\RelWithDebInfo
```

## 2. Build results — PASS; compile/link logs zero-warning; two out-of-tree configure warnings listed
| Step | Exit | Cache | warning/error lines in compile/link log |
|---|---|---|---|
| Clean configure (build_x64 deleted first) | 0 | PERF_LOG OFF | (configure: 2, listed below) |
| Build (OFF) | 0 |  | 0 / 0 |
| Reconfigure + build (PERF_LOG=ON) | 0 / 0 | PERF_LOG ON | 0 / 0 |
| Reconfigure + rebuild (PERF_LOG=OFF, left OFF) | 0 / 0 | PERF_LOG OFF (final state) | 0 / 0 |

Configure warnings (both from the OBS-studio dependency configure inside `.deps`, not
plugin code; same two as M4-M7 — listed, not ignored):
1. `CMake Warning (dev) at cmake/finders/FindDetours.cmake:65: Failed to find detours
   version.` (obs-studio sources, win-capture/graphics-hook)
2. `CMake Warning at plugins/win-dshow/virtualcam-module/CMakeLists.txt:14: Empty
   Virtual Camera GUID set.` (obs-studio sources)

Version (buildspec-driven) — CONFIRMED 0.2.0:
- buildspec.json `"version": "0.2.0"`
- CMakeCache.txt: `CMAKE_PROJECT_VERSION:STATIC=0.2.0`
- Shipped DLL resource: `FileVersion=0.2.0 ProductVersion=0.2.0 Product=streamsentry`

## 3. Test results — PASS
ctest (`-C RelWithDebInfo`): **4/4 passed, 0 failed** (coord-map 0.03s, plate-gen 0.14s,
toast-gate 0.02s, frame-decide 0.02s). Direct runs: all four print "all passed";
the suites are silent-on-pass, so per-suite assertion counts are static CHECK-site
counts: coord-map 36 · plate-gen 25 · toast-gate 24 · frame-decide 56 (= 141 CHECKs).

watcher-selftest.exe (not add_test-registered by design; run explicitly), exit 0 —
**all 11 deterministic legs ok**, including the NEW M8 leg:
```
ok:   watcher thread started
ok:   heartbeat advances while watcher alive
ok:   launched notepad.exe
ok:   blocklist rect appeared after notepad opened
ok:   blocklist rect disappeared after notepad closed
ok:   picker enumerates open windows (deduped, named)      <- NEW (M8, SPEC 2.5)
info: picker sees 7 distinct processes
ok:   allowlist mode masks unapproved windows (rects or mask_all)
ok:   blocklist mode restored after switching back
ok:   heartbeat frozen while killed (>500ms) -> render shows protection-degraded chip
ok:   heartbeat resumes after un-kill
ok:   watcher thread stopped cleanly
watcher-selftest: all deterministic checks passed
```
Toast leg: **INCONCLUSIVE** — "no toast banner detected (likely Do Not Disturb / focus
assist suppressing the banner)" — same documented outcome as M2-M7
(reports/M2-DECISIONS.md); real toast masking remains on the human checklist.
Cleanup verified: no notepad processes left after the run.

## 4. Packaging dry-run — PASS (fresh; prior release/ tree deleted first)
Install exit 0. Installed tree (exactly these files, nothing extra):
```
streamsentry/bin/64bit/streamsentry.dll
streamsentry/bin/64bit/streamsentry.pdb
streamsentry/data/locale/en-US.ini
```
Zip recreated from the fresh install tree: release\streamsentry-0.2.0-windows-x64.zip
Entries (3 file + 2 dir):
```
streamsentry\bin\64bit\streamsentry.dll  77824 bytes
streamsentry\bin\64bit\streamsentry.pdb  1798144 bytes
streamsentry\data\locale\en-US.ini       1013 bytes
```
- Zipped DLL is byte-identical to the final PERF_LOG=OFF build: SHA256
  `51273c1a220e0a18b7190fc15f9fcef143f155503037517cefdeca29c78564b4` (both files).
- Shipping hygiene: ASCII scan of the zipped DLL for "PERF watcher tick" = 0 matches
  (OFF variant shipped).
- README consistency: README names `streamsentry-0.2.0-windows-x64.zip` and states the
  zip mirrors `streamsentry/bin/64bit/...` + `streamsentry/data/...` — MATCHES the
  actual zip layout verbatim.
- git-ignore: `git check-ignore -v` matches release/, build_x64/, .deps/,
  CMakeUserPresets.json (all via `.gitignore:2:/*`); the recreated release/ tree and
  zip do NOT appear in `git status`.

## 5. Artifacts — all present (build_x64/RelWithDebInfo, from the final OFF rebuild)
`streamsentry.dll` (77824) + `streamsentry.pdb` (1798144) · `coord-map-tests.exe` ·
`plate-gen-tests.exe` · `toast-gate-tests.exe` · `frame-decide-tests.exe` ·
`watcher-selftest.exe` (+ `w32-pthreads.dll` copied beside it, post-build rule).

## 6. Deployment note (informational, per task — NOT a pass/fail item)
The owner's OBS at D:\software\obs\obs-studio carries an M8 **perf** build + locale,
deployed before this verification run:
- obs-plugins\64bit\streamsentry.dll: 80384 bytes, 2026-07-10 16:53, ProductVersion
  0.2.0, "PERF watcher tick" token count = 1 -> PERF_LOG=ON variant, consistent with
  "M8 perf build" as described (.pdb alongside).
- data\obs-plugins\streamsentry\locale\en-US.ini: 1013 bytes, 16:48 — byte-identical
  to the staged repo locale (diff clean).
This deployed DLL is intentionally NOT the release-zip DLL (perf variant for the
owner's v0.2 perf re-measurement); the zip ships the OFF build verified above.

## 7. Not covered by automation (explicit)
- **In-OBS picker UI behavior**: combo populating inside the real properties dialog,
  clicking "Add selected window to the active list" appending to the correct list,
  no-duplicate on double-add, refresh re-scan, and the mask appearing after a
  picker-only add (SPEC 2.5 acceptance + the "Picker: add open window, no typing" row
  of the SPEC manual matrix) — owner-manual, in OBS.
- **README accuracy as rendered** (install/usage walkthrough against the real UI) —
  owner-manual.
- Real toast masking end-to-end (self-test leg INCONCLUSIVE under system DND), the
  SPEC manual test matrix (allowlist/panic/mode-switch rows), multi-monitor/DPI cases,
  and the v0.2 perf re-measurement on the deployed perf build — all owner-manual per
  TESTING.md / SPEC.md.
- Unit suites cover pure modules only (coord-map, plate-gen, toast-gate, frame-decide);
  watcher-selftest covers the live watcher + picker enumeration without obs.dll. No
  automated test exercises libobs UI callbacks (picker buttons) — those are
  code-reviewed + owner-manual only.

## 8. Final verdict
**VERIFIED** — clean 0.2.0 build zero-warning (OFF and ON variants, cache left OFF);
ctest 4/4; watcher self-test 11/11 deterministic legs ok including the new picker leg
(toast INCONCLUSIVE, documented); fresh packaging dry-run layout matches README
verbatim with a byte-identical DLL; release/ git-ignored. In-OBS picker interaction
remains manual.
