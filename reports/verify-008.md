# Release verification 008 - owner removal of panic hotkey

- Run: 2026-07-15 23:08 +08:00, Windows 11 10.0.26200
- Starting HEAD: `7adcf43ed3b8037657463a5efbfdf668e91df1d2`
- Scope: the complete current workspace candidate before this report, including
  the owner ruling and retained publication-gate history
- Toolchain: CMake 3.28.0-rc5; Visual Studio 2022 17.14.28 / MSVC
  19.44.35224.0; Windows SDK 10.0.26100.0; OBS dependency sources 31.1.1
- Configuration: Windows x64 `Release`,
  `CMAKE_COMPILE_WARNING_AS_ERROR=ON`
- Final ZIP:
  `F:\StreamSentry\release\streamsentry-0.2.0-windows-x64.zip`
- **Verdict: VERIFIED**

## 1. Frozen workspace fingerprint

Starting and final non-ignored project status, before this report was created:

```text
 M .gitignore
 M AGENTS.md
 M ARCHITECTURE.md
 M CHANGELOG.md
 M CLAUDE.md
 M README.md
 M SPEC.md
 M TESTING.md
 M data/locale/en-US.ini
 M src/filter.c
?? docs/INSTALLATION.md
?? docs/PUBLISHING_SOP.md
?? reports/RULING-2026-07-15-remove-panic-hotkey.md
?? reports/spec-review-005.md
?? reports/spec-review-006.md
?? reports/verify-005.md
?? reports/verify-006.md
?? reports/verify-007.md
```

Local `master`, `origin/master`, and remote `master` resolve to
`7adcf43ed3b8037657463a5efbfdf668e91df1d2`.

Tracked diff: 10 files, 261 insertions, 117 deletions. Canonical binary-patch
fingerprint:

```powershell
cmd /d /c "git diff --binary --no-ext-diff HEAD | git hash-object --stdin"
# 85e299f92c691e810014117e58252e98f3e8f260
```

All untracked inputs except this new report are part of the candidate:

| File | Git blob | SHA-256 |
|---|---|---|
| `docs/INSTALLATION.md` | `2eabf8b342f80dadbfe1c7f23745a61864c272eb` | `8d9bc3fc19041650268a371cbd972771f7412fc47f5924cd071b28a52f31bd1d` |
| `docs/PUBLISHING_SOP.md` | `147c89396ebeed9fc4f894e705ff64b9e2ad0511` | `e945c592999264debd9604a59d08ec001b69ad863be3db2ee7a12b17616ef797` |
| `reports/RULING-2026-07-15-remove-panic-hotkey.md` | `a1b03ef26ba3d3098f97b5aeecb6e8ae9e7b95d4` | `2c426d6a591f2e78a051daff45acd186777af72a9686fdfcb6c1315eb7dc53c8` |
| `reports/spec-review-005.md` | `f02f9354eed9b6c748d7ecf4f22733db4c3e993b` | `ba2c713ccb0617f0b5e01af01a5392ca248ffeb8867aff2cb9da37966730e026` |
| `reports/spec-review-006.md` | `43025c8edfabd42004fc119f2af7c9716456c3cd` | `bef041b8fd0663662d26e8a752b512bc5ee9db64e4b0861c2860126d069709b8` |
| `reports/verify-005.md` | `1d61e1d8037adf76bf30018138e895831ff4741f` | `bd9cb4354c9c0c0b0bac17ca96c3aed4ec2d087753d5dd3671465b3fc00d2028` |
| `reports/verify-006.md` | `52d168630d9c5e4f654fc66ac013cff861cd6c06` | `ffb8e19746fbbad6a5248e79e37bcd95e2dece0fdf39e7ae7614dbfc67ed2840` |
| `reports/verify-007.md` | `b59dc6fdc380acbf923e9a177cae1e208bd0bd6e` | `b82d74b3e31946a6307a6bb7939aea9de5aa233cb6271ab510426e4fac937238` |

The next spec guardian must recompute this exact state while excluding only
this new verifier report. Any post-verifier source, locale, specification,
public-document, workflow, ruling, TESTING, CHANGELOG, or retained-report
change invalidates this gate.

## 2. Clean configure and Release build

The ignored `F:\StreamSentry\build_x64` and `F:\StreamSentry\release`
paths were resolved and confirmed to be the intended workspace children before
both were recursively removed.

```powershell
cmake --preset windows-x64-local -DCMAKE_COMPILE_WARNING_AS_ERROR=ON
cmake --build build_x64 --config Release --parallel -- /consoleLoggerParameters:Summary /noLogo
```

Result: **PASS**. Clean configure and Release build exited 0. StreamSentry
reported 0 compiler warnings and 0 errors. The cache confirms version 0.2.0,
Visual Studio 2022 x64 generation, and warnings-as-errors enabled. Configure
selected Windows SDK 10.0.26100.0.

The two configure warnings are from OBS dependency sources, not StreamSentry:

1. `FindDetours.cmake:65`: failed to find the Detours version.
2. `win-dshow/virtualcam-module/CMakeLists.txt:14`: empty Virtual Camera GUID.

## 3. Automated and direct tests

```powershell
ctest --test-dir build_x64 -C Release --output-on-failure
build_x64\Release\coord-map-tests.exe
build_x64\Release\plate-gen-tests.exe
build_x64\Release\toast-gate-tests.exe
build_x64\Release\frame-decide-tests.exe
build_x64\Release\watcher-selftest.exe
```

| Test | Result |
|---|---|
| CTest aggregate | PASS - 4/4, 0 failed, 0.19 s |
| coord-map direct | PASS, exit 0 |
| plate-gen direct | PASS, exit 0 |
| toast-gate direct | PASS, exit 0 |
| frame-decide direct | PASS, exit 0 |
| watcher self-test | PASS, exit 0 |

The watcher self-test passed heartbeat, Notepad blocklist appearance and
disappearance, deduplicated named picker enumeration (8 processes), allowlist
masking, blocklist restoration, debug-kill degradation, recovery, and clean
shutdown. It left zero Notepad processes.

The real-toast leg remained **INCONCLUSIVE** because Windows displayed no
banner, consistent with the documented Do Not Disturb / Focus Assist condition.

## 4. Allowlist mask-all regression check

Result: **PASS**.

- `src/frame-decide.c` retains `SS_ALLOWLIST_FALLBACK()` and sets
  `out->mask_all = true` for every unverified allowlist path: missing/stale
  snapshot, mode transition, detection degradation, unresolved geometry, and
  mapping failure.
- `src/filter.c` retains `if (dec.mask_all)` as the full-source opaque
  plate-instead-of-target render path and stacks the degraded chip when the
  decision is unverified.
- `frame-decide-tests.exe` still passes all allowlist cases directly, including
  stale, missing snapshot, transition, unresolved geometry, invalid mapping,
  detection degradation, and overflow.
- Blocklist unverified behavior remains source rendering plus the degraded chip;
  the current README and installation SOP still distinguish the two modes.

Removing the user-triggered control did not remove, weaken, or rename the
deterministic allowlist default-deny fallback.

## 5. Clean install, exact archive, version, and hashes

```powershell
cmake --install build_x64 --prefix "$PWD\release\Release" --config Release
Compress-Archive -Path (Get-ChildItem release\Release).FullName `
  -DestinationPath release\streamsentry-0.2.0-windows-x64.zip `
  -CompressionLevel Optimal -Force
```

Result: **PASS**. Install tree and archive contain one `streamsentry` root and
exactly:

```text
streamsentry/bin/64bit/streamsentry.dll  76288 bytes
streamsentry/data/locale/en-US.ini        1377 bytes
```

No PDB or extra file is present. The installed DLL is byte-identical to the
built DLL, and installed locale is byte-identical to source. DLL FileVersion
and ProductVersion are 0.2.0, ProductName is `streamsentry`, and Authenticode
status is `NotSigned`.

| Artifact | SHA-256 |
|---|---|
| Release DLL (built and installed) | `a9da896362379153953ebf05d454504956ff776a57ea512a7a57b402b315fb2d` |
| `F:\StreamSentry\release\streamsentry-0.2.0-windows-x64.zip` | `5d1e7d8779899c9b21b46e0b6946f82504ef14ffa232174e47b4a8ca26710018` |
| source and installed `en-US.ini` | `51a1cf65ef10bae9ec9db41a2955db9724719f67254b5d5617b2eeae05835d75` |
| coord-map test executable | `3fb46dfdf3014e1b18f8e772d930d483b2afcdadeed4d2a5e133fb9ecae971f6` |
| plate-gen test executable | `876a4d3a13c0b08aa279cc689bf17170c960df7490944dc2311008a24cea858b` |
| toast-gate test executable | `65a3c702022f45237ed25cefbf0dd8baf56aead5e128221e3a3adb779b4abb86` |
| frame-decide test executable | `a6aefc4360f7736aec0c71c9c39db1ea678a325b64dcaa27a6edae21243b8bbb` |
| watcher self-test executable | `62b3e777fdaa67886cc2ca7f99f23cbdcac934c433032f2d256d8d09bd41e64a` |

The DLL decreases from the previous 77,312 bytes to 76,288 bytes, and locale
from 1,429 bytes to 1,377 bytes, consistent with removing code/state/log
strings and the one locale entry. Pure test executable hashes remain identical
to the prior gate because their source did not change.

## 6. No shipping panic/hotkey surface

Result: **PASS**.

### Source and locale

An in-memory case-insensitive scan of every file below `src/` and
`data/locale/` found zero occurrences of:

```text
panic
hotkey
obs_hotkey
PanicHotkey
streamsentry.panic
```

`src/filter.c` has no panic boolean, hotkey ID, callback, registration,
unregistration, atomic toggle, panic logs, or panic render branch. The only
full-source branch is `dec.mask_all`. `data/locale/en-US.ini` has no
`PanicHotkey` key or user-facing panic label.

### Release DLL

The built/installed Release DLL was scanned as ASCII and UTF-16 for `panic`,
`hotkey`, `streamsentry.panic`, `PanicHotkey`, and `mask everything`: every
token returned absent. MSVC `dumpbin /imports` also returned zero panic/hotkey
import hits. Thus the shipping binary contains no StreamSentry hotkey
registration symbol, state label, callback label, log string, or UI text.

### Current user and publishing documentation

A case-insensitive scan of `README.md`, `docs/INSTALLATION.md`, and
`docs/PUBLISHING_SOP.md` returned zero panic/hotkey occurrences. The first-run
smoke test now covers loading, Blocklist picker/masking, removal, and log
inspection without asking a user to bind or exercise a control that does not
ship. All GitHub workflows, composite actions, and packaging scripts also
returned zero panic/hotkey occurrences.

### Current governance and as-built documentation

- `AGENTS.md` and `CLAUDE.md` define four shipping v0.2 items and record that
  the experimental hotkey was removed by owner ruling on 2026-07-15.
- `SPEC.md` converts section 2.4 into the explicit removal decision and requires
  no registration, state, label, or workflow while preserving allowlist
  mask-all.
- `ARCHITECTURE.md` removes the former state/callback/render description and
  describes the full-source branch only as allowlist mask-all.
- `reports/RULING-2026-07-15-remove-panic-hotkey.md` records the owner decision
  and the unchanged allowlist/blocklist failure semantics.

Historical M7 sections in TESTING/CHANGELOG and old gate reports remain
evidence for earlier fingerprints. The only TESTING change is the prior
publication-gate section appended after the older history; the CHANGELOG diff
is confined to the prior publication-readiness entry at the top. No old M7
section or old report was edited by this verifier. A later gated scribe should
append the new removal evidence and replace panic with the current non-panic
smoke/manual matrix in the newest release-status entry; the shipping user docs
and artifact are already consistent for this verifier fingerprint.

## 7. Markdown, package/UI, and repository consistency

Result: **PASS**.

- `git diff --check` passes.
- An in-memory audit checked all 58 current Markdown files and resolved all 22
  local links and anchors. No missing target or bad local anchor was found.
- README, installation SOP, package name, ProgramData layout, custom/portable
  mapping, Windows x64/OBS 32.1.2 evidence, versioned load log, unsigned status,
  update/uninstall steps, and exact two-file package agree.
- Current locale labels are limited to filter enable/mode, blocklist,
  allowlist, picker, refresh, and add controls. `src/filter.c` wires those
  labels and exposes no removed control.
- AGENTS and CLAUDE agree on the owner removal and four-item shipping scope;
  SPEC and ARCHITECTURE agree with source and locale; the owner ruling preserves
  allowlist mask-all and Blocklist degraded behavior.
- No new dependency or out-of-scope runtime feature appears. The plugin still
  links libobs plus Windows SDK libraries only, with frontend API and Qt off.

## 8. GitHub publishing SOP and workflows

Result: **PASS**.

- Phase headings remain the exact sequence 0 through 11: candidate,
  verification, sequential gate, exact-package acceptance, single scribe,
  commit/push, tag/draft, download-back, visibility, publication, anonymous
  check.
- The tag run correctly treats format as skipped and binds format evidence to
  the exact commit's preceding `master` push.
- `0.2.0-beta1` still selects Release configuration and a draft prerelease.
  The Windows job produces the documented ZIP; the release job downloads it,
  generates SHA-256 notes, and uses `draft: true` plus `prerelease: true`.
- Commit, push, tag push, visibility, and publication remain separately gated
  by explicit owner authorization. Nothing in the hotkey removal alters the
  publication or packaging path.

## 9. Remaining manual and post-authorization checks

- Install this exact final ZIP into the supported ProgramData path, confirm OBS
  32.1.2 loads version 0.2.0 from that path, add the filter, and complete the
  current non-panic Blocklist picker/masking smoke test.
- Confirm in OBS that no StreamSentry hotkey appears under Settings -> Hotkeys.
- Exercise real Blocklist unverified source-plus-chip and Allowlist unverified
  full-source-plate-plus-chip behavior and recovery.
- Verify real Windows toast timing, blocklist/allowlist picker behavior,
  taskbar and wallpaper/sliver exclusions, and mixed-DPI/multi-monitor
  placement.
- Complete the 30-minute SRT soak/watcher p99, two-hour memory stability,
  60 fps impact, and fresh-machine unsigned-warning UX.
- After explicit authorization, confirm tag Actions/draft creation, download
  back the GitHub ZIP, validate checksum/exact layout, retest OBS, accept public
  history/privacy exposure, change visibility, publish the prerelease, and run
  anonymous repository/asset/Issues checks.

## 10. Final verdict and write boundary

**VERIFIED.** The exact current workspace fingerprint passes a clean
warnings-as-errors Windows x64 Release build, all automated and direct watcher
tests, exact no-PDB package/version/signature/hash checks, Markdown links,
package/UI/current-doc consistency, GitHub SOP/workflow order, complete removal
of the panic hotkey from shipping source/locale/DLL/user workflow, and
preservation of allowlist mask-all behavior.

This verifier wrote only `reports/verify-008.md` in the project tree, plus the
explicitly required ignored build/package artifacts below `build_x64` and
`release`. It did not edit source, SPEC, governance files, architecture,
README, docs, TESTING, CHANGELOG, locale, workflows, owner ruling, or any
existing report. It did not launch OBS, stage, commit, push, tag, publish, or
change repository visibility.
