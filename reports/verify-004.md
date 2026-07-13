# Release verification 004 — README repair

- Run: 2026-07-13 23:55 +08:00, Windows 11 10.0.26200
- Starting HEAD: `0be2ec9bfe2956e32fbb38f74c79ae390eed269d`
  (`Add honest AI-authorship disclosure to README`)
- Scope: complete current uncommitted release-preparation state, including all
  retained failed audits and verify-001/002/003, before this report was created
- Toolchain: CMake 3.28.0-rc5; Visual Studio 2022 / MSVC 19.44.35224.0;
  Windows SDK 10.0.26100.0; OBS dependency sources 31.1.1
- Configuration: Windows x64 `Release`,
  `CMAKE_COMPILE_WARNING_AS_ERROR=ON`
- **Verdict: VERIFIED**

## 1. Freshness fingerprint

Starting `git status --porcelain=v1`:

```text
 M .github/workflows/build-project.yaml
 M .gitignore
 M ARCHITECTURE.md
 M CLAUDE.md
 M README.md
 M SPEC.md
 M buildspec.json
 M cmake/windows/helpers.cmake
 M data/locale/en-US.ini
 M src/watcher.cpp
 M src/watcher.h
?? AGENTS.md
?? reports/RULING-2026-07-13-noncontent-window-exclusions.md
?? reports/spec-review-002.md
?? reports/spec-review-003.md
?? reports/verify-001.md
?? reports/verify-002.md
?? reports/verify-003.md
```

Tracked diff against HEAD: 11 files, 111 insertions, 65 deletions. Canonical
binary-patch fingerprint:

```powershell
cmd /d /c "git diff --binary --no-ext-diff HEAD | git hash-object --stdin"
# 7b514f08577e0873957b48aae0a132f2fc7bd69a
```

Untracked input fingerprints:

| File | Git blob | SHA-256 |
|---|---|---|
| `AGENTS.md` | `ee740f9e5256efc0aa6e1a17cd34ff445e4ee724` | `3f876e77bcba0dd79714c55ee335064ff2f38a3bc37184d88db95196e20f80f2` |
| `reports/RULING-2026-07-13-noncontent-window-exclusions.md` | `297afe5f46324ca0f2ea2e837f373449a50081d9` | `b63bb1cdb9f903f082a01e4d8a42b1c937f140b1d2ef801a7e51447e42f6cdc1` |
| `reports/spec-review-002.md` | `061e38b241174883fdf2feacb5f80db336acd27a` | `1260942cf5adaea8d832c8414172bd5e27fce716aecd5fd68fc14e9988e4b26a` |
| `reports/spec-review-003.md` | `72ccf39bc6a4e5292dd73e2c9403cc9106104361` | `98d4f48db10d25d7e7dec80ac14ceeac98c5b66ed47fa36e87493c2d4b608eb2` |
| `reports/verify-001.md` | `aa85bf8d4d9864f1234d5d4cadaee4e02ca0c1b8` | `b5a3873669641afea4dfaecdf85784fd4086d8f9d1a997cea42631c1ee9aa796` |
| `reports/verify-002.md` | `dc9e43584dfe90c0cb3c6b4e267622c415d3a877` | `f6dc2dd196ee8196845e6bd9e2cc56f4c70c9b403a06965d5a368e47fc0683b1` |
| `reports/verify-003.md` | `8a01cf5b823a116485bea18f554a4ce6853969bb` | `9918053b1610cd9f81ea66184239dd0d4ae3957c1efcac60be8ade2bf941b483` |

The next spec-guardian must recompute this state while excluding only this new
report. Any other post-verification project change invalidates the result.

## 2. Clean configure and Release build

The ignored `F:\StreamSentry\build_x64` and `F:\StreamSentry\release`
directories were first resolved beneath the workspace and deleted.

```powershell
cmake --preset windows-x64-local -DCMAKE_COMPILE_WARNING_AS_ERROR=ON
cmake --build build_x64 --config Release --parallel -- /consoleLoggerParameters:Summary /noLogo
```

Result: **PASS**. Clean configure exit 0; build exit 0; 0 compiler warnings;
0 compiler errors. Cache confirms project version 0.2.0, x64 generator, and
warnings-as-errors enabled.

Two configure warnings are from OBS dependency sources, not StreamSentry:

1. `FindDetours.cmake:65`: failed to find Detours version.
2. `win-dshow/virtualcam-module/CMakeLists.txt:14`: empty Virtual Camera GUID.

The documented local-SDK preset was used for SDK 10.0.26100. PowerShell Core is
not installed on this host, so the CI-only PowerShell 7 wrapper was not invoked;
the supported local CMake path and the same underlying install/archive
operations were run directly. No dependency was installed.

## 3. Automated tests

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
| CTest aggregate | PASS — 4/4, 0 failed, 0.17 s |
| coord-map direct | PASS, exit 0 |
| plate-gen direct | PASS, exit 0 |
| toast-gate direct | PASS, exit 0 |
| frame-decide direct | PASS, exit 0 |
| watcher self-test | PASS, exit 0 |

The watcher self-test passed start/heartbeat, Notepad blocklist appearance and
disappearance, deduplicated named picker enumeration (8 processes), allowlist
masking, blocklist restoration, debug-kill degradation, recovery, and clean
shutdown. It left zero Notepad processes.

The toast leg remained **INCONCLUSIVE** because no banner was displayed,
consistent with the documented Do Not Disturb / Focus Assist condition. The
real-toast timing check remains owner-manual.

## 4. Clean install, package, and no-PDB check

```powershell
cmake --install build_x64 --prefix F:\StreamSentry\release\Release --config Release
Compress-Archive -Path (Get-ChildItem release\Release).FullName `
  -DestinationPath release\streamsentry-0.2.0-windows-x64.zip `
  -CompressionLevel Optimal -Force
```

Exact installed files:

```text
streamsentry/bin/64bit/streamsentry.dll  77312 bytes
streamsentry/data/locale/en-US.ini        1429 bytes
```

Archive: **PASS**. One `streamsentry` root, exactly the same two files,
installed PDB count 0, archive PDB count 0. Installed DLL equals the built DLL.
FileVersion/ProductVersion are 0.2.0, ProductName is `streamsentry`, and
Authenticode status is `NotSigned`.

Artifact SHA-256 values:

| Artifact | SHA-256 |
|---|---|
| Release DLL (built and installed) | `3ac9946d2fa4ef53b2aba6af10e7c31b2a9257c8bdea9e72e398905f799c24d4` |
| `streamsentry-0.2.0-windows-x64.zip` | `6f182c86ccea862d85463bc0ba70aad97ebc8521a9966478561b3d9b7824eb52` |
| source and installed `en-US.ini` | `2684ebd9ff6d56e1464efb4899f12da449e5ee143e95c2dd0b355c0dcf73e6a9` |
| coord-map test executable | `3fb46dfdf3014e1b18f8e772d930d483b2afcdadeed4d2a5e133fb9ecae971f6` |
| plate-gen test executable | `876a4d3a13c0b08aa279cc689bf17170c960df7490944dc2311008a24cea858b` |
| toast-gate test executable | `65a3c702022f45237ed25cefbf0dd8baf56aead5e128221e3a3adb779b4abb86` |
| frame-decide test executable | `a6aefc4360f7736aec0c71c9c39db1ea678a325b64dcaa27a6edae21243b8bbb` |
| watcher self-test executable | `62b3e777fdaa67886cc2ca7f99f23cbdcac934c433032f2d256d8d09bd41e64a` |

DLL, locale, and test hashes match verify-003; the new zip hash reflects fresh
archive timestamps.

## 5. README, package, and UI consistency

Result: **PASS**.

- The picker instruction now matches
  `Pick one, press **Add to list**, and it lands ...` across the wrapped lines.
  A direct normalized-whitespace regex matched it; searches found zero malformed
  `press *Add` / duplicated Add fragments. The README contains 102 `**` tokens,
  an even count, and `git diff --check` exits 0.
- The documentation-guide sentence now matches
  `Claude Code and Codex workflows that build this plugin`; the stale plural
  `workflows that builds` pattern occurs zero times.
- README names **Add to list** and **Refresh list**. The shipped locale defines
  exactly `PickerAdd="Add to list"` and `PickerRefresh="Refresh list"`, and
  `src/filter.c` wires both keys into the actual OBS property buttons.
- The locale read directly from the zip retains the corrected content-bearing
  allowlist description and deterministic non-content exclusion notice.
- Package name/layout, Windows 10 21H2+ requirement, ProgramData install path,
  release-note checksum wording, log string, 0.2.0 version, unsigned warning,
  and repository links remain consistent with the artifact and workflow.

The malformed README and grammar findings in retained `spec-review-003.md` are
fixed. That failed audit remains unchanged as history and requires a new spec
review for this exact fingerprint.

## 6. Manual checks still uncovered

- Real Windows toast signature/geometry/timing before content is readable.
- In-OBS allowlist, blocklist, picker, panic, degraded chip/mask-all, and real
  wallpaper/sliver exclusions.
- Multi-monitor / mixed-DPI placement and supported full-monitor Display
  Capture geometry.
- 30-minute SRT soak with zero stale events and watcher p99; two-hour idle
  memory stability; 60 fps render impact.
- Fresh-machine install/uninstall/security-warning UX and rendered README/UI
  accuracy.

## 7. Final verdict

**VERIFIED.** Clean warnings-as-errors Windows x64 Release build, all automated
unit/watcher tests, clean two-file no-PDB package, artifact identity, packaged
locale, corrected Markdown instruction, grammar, and exact README/UI labels all
pass for the fingerprint above. Manual acceptance remains explicitly open. The
sequential spec-guardian may audit this same state; any post-report project
change invalidates this result. No stage, commit, push, or publish occurred.
