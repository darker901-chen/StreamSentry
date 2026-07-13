# Release verification 002 — v0.2.0 beta preparation

- Run: 2026-07-13 23:38 +08:00, Windows 11 10.0.26200
- Starting HEAD: `0be2ec9bfe2956e32fbb38f74c79ae390eed269d`
  (`Add honest AI-authorship disclosure to README`)
- Scope: complete current uncommitted release-preparation diff, including the
  retained FAILED `reports/verify-001.md`, before this report was created
- Toolchain: CMake 3.28.0-rc5; Visual Studio 2022 / MSVC 19.44.35224.0;
  Windows SDK 10.0.26100.0; OBS dependency sources 31.1.1
- Configuration: x64 `Release`, `CMAKE_COMPILE_WARNING_AS_ERROR=ON`
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
 M src/watcher.cpp
?? AGENTS.md
?? reports/RULING-2026-07-13-noncontent-window-exclusions.md
?? reports/verify-001.md
```

Tracked diff against HEAD: 9 files, 98 insertions, 53 deletions. Canonical
binary-patch fingerprint:

```powershell
cmd /d /c "git diff --binary --no-ext-diff HEAD | git hash-object --stdin"
# 1c36b1d81fba0d5cf9428da780ff37b5c11e746f
```

Untracked input fingerprints:

| File | Git blob | SHA-256 |
|---|---|---|
| `AGENTS.md` | `ee740f9e5256efc0aa6e1a17cd34ff445e4ee724` | `3f876e77bcba0dd79714c55ee335064ff2f38a3bc37184d88db95196e20f80f2` |
| `reports/RULING-2026-07-13-noncontent-window-exclusions.md` | `297afe5f46324ca0f2ea2e837f373449a50081d9` | `b63bb1cdb9f903f082a01e4d8a42b1c937f140b1d2ef801a7e51447e42f6cdc1` |
| `reports/verify-001.md` | `aa85bf8d4d9864f1234d5d4cadaee4e02ca0c1b8` | `b5a3873669641afea4dfaecdf85784fd4086d8f9d1a997cea42631c1ee9aa796` |

The spec-guardian must recompute these values while excluding this report. Any
other post-verification source, documentation, workflow, ruling, or prior-report
change invalidates this evidence.

## 2. Clean configure and Release build

The ignored `F:\StreamSentry\build_x64` and `F:\StreamSentry\release`
directories were first resolved beneath the workspace and recursively deleted.

Commands:

```powershell
cmake --preset windows-x64-local -DCMAKE_COMPILE_WARNING_AS_ERROR=ON
cmake --build build_x64 --config Release --parallel -- /consoleLoggerParameters:Summary /noLogo
```

Result: **PASS**. Clean configure exit 0; build exit 0; 0 compiler warnings; 0
compiler errors. `CMakeCache.txt` confirms project version 0.2.0, x64 generator
platform, and warnings-as-errors enabled.

Two configure warnings came from the OBS 31.1.1 dependency sources rather than
StreamSentry:

1. `FindDetours.cmake:65`: failed to find Detours version.
2. `win-dshow/virtualcam-module/CMakeLists.txt:14`: empty Virtual Camera GUID.

The local preset is the README-documented fallback for this host's installed
10.0.26100 SDK and ignored dependency cache. PowerShell Core is not installed
on this verifier host, so the CI-only PowerShell 7 wrapper was not invoked;
the documented local CMake build plus its underlying install/archive operations
were exercised directly. No dependency was installed.

## 3. Automated tests

Commands:

```powershell
ctest --test-dir build_x64 -C Release --output-on-failure
build_x64\Release\coord-map-tests.exe
build_x64\Release\plate-gen-tests.exe
build_x64\Release\toast-gate-tests.exe
build_x64\Release\frame-decide-tests.exe
build_x64\Release\watcher-selftest.exe
```

Results:

| Test | Result |
|---|---|
| CTest aggregate | PASS — 4/4, 0 failed, 0.18 s |
| coord-map direct | PASS, exit 0 |
| plate-gen direct | PASS, exit 0 |
| toast-gate direct | PASS, exit 0 |
| frame-decide direct | PASS, exit 0 |
| watcher self-test | PASS, exit 0 |

The watcher self-test passed every deterministic leg: thread start and
heartbeat, Notepad blocklist appearance/disappearance, deduplicated named
picker enumeration (7 processes), allowlist masking, blocklist restoration,
debug-kill stale-heartbeat degradation behavior, heartbeat recovery, and clean
shutdown. It left zero Notepad processes behind.

The toast leg remained **INCONCLUSIVE** because no banner was displayed,
consistent with the documented Do Not Disturb / Focus Assist condition. The
real-toast timing check remains owner-manual and does not fail the deterministic
self-test.

## 4. Clean install and package dry run

Commands (the underlying operations used by the Windows package script):

```powershell
cmake --install build_x64 --prefix F:\StreamSentry\release\Release --config Release
Compress-Archive -Path (Get-ChildItem release\Release).FullName `
  -DestinationPath release\streamsentry-0.2.0-windows-x64.zip `
  -CompressionLevel Optimal -Force
```

Install result: **PASS**. Exact installed files:

```text
streamsentry/bin/64bit/streamsentry.dll  77312 bytes
streamsentry/data/locale/en-US.ini        1276 bytes
```

Archive result: **PASS**. It has one root folder (`streamsentry`) and exactly
two file entries:

```text
streamsentry\bin\64bit\streamsentry.dll  77312 bytes
streamsentry\data\locale\en-US.ini        1276 bytes
```

Installed PDB count: 0. Archive PDB count: 0. The installed DLL is
byte-identical to the built Release DLL. FileVersion/ProductVersion are 0.2.0;
ProductName is `streamsentry`; Authenticode status is `NotSigned`.

Artifact SHA-256 values:

| Artifact | SHA-256 |
|---|---|
| Release DLL (built and installed) | `3ac9946d2fa4ef53b2aba6af10e7c31b2a9257c8bdea9e72e398905f799c24d4` |
| `streamsentry-0.2.0-windows-x64.zip` | `e269fc02b8ba7cb0c5305094c9283c6c8c425c6eb53da34c759288e23bc1e015` |
| installed `en-US.ini` | `65757c15e4c77c7d21d0da0049d7528226e094d13e64634ce17e276a1bdf2999` |
| coord-map test executable | `3fb46dfdf3014e1b18f8e772d930d483b2afcdadeed4d2a5e133fb9ecae971f6` |
| plate-gen test executable | `876a4d3a13c0b08aa279cc689bf17170c960df7490944dc2311008a24cea858b` |
| toast-gate test executable | `65a3c702022f45237ed25cefbf0dd8baf56aead5e128221e3a3adb779b4abb86` |
| frame-decide test executable | `a6aefc4360f7736aec0c71c9c39db1ea678a325b64dcaa27a6edae21243b8bbb` |
| watcher self-test executable | `62b3e777fdaa67886cc2ca7f99f23cbdcac934c433032f2d256d8d09bd41e64a` |

The zip hash naturally differs from verify-001 because archive entry timestamps
are regenerated; the built DLL and all executable hashes are identical.

## 5. README and release-claim validation

Result: **PASS**.

- Windows requirement now says Windows 10 21H2 or later / Windows 11 x64,
  matching SPEC Part 1.
- Package filename and the documented `streamsentry/bin/64bit` plus
  `streamsentry/data/locale` bundle layout exactly match the dry-run archive.
- `C:\ProgramData\obs-studio\plugins` and the documented nested bundle match
  the official OBS Plugins Guide's recommended Windows manual-install layout.
- The GitHub remote, buildspec website, Releases link, and Issues link all use
  `darker901-chen/StreamSentry`.
- README now tells users to compare the zip SHA-256 with the checksum printed
  in the release notes. The tag workflow generates `CHECKSUMS.txt` and passes
  it as the draft release `body_path`, so this is the actual delivery behavior.
- Source contains the exact successful-load message and local OBS logs show OBS
  32.1.2 x64 loading StreamSentry 0.2.0 successfully on 2026-07-10.
- The archive DLL is unsigned, matching the README warning.
- Release configuration omits the PDB as intended.

The two blockers from retained `verify-001.md` are resolved. The FAILED report
remains unmodified as an audit trail and is superseded only for this exact
freshness fingerprint by this VERIFIED report.

## 6. Manual checks still uncovered

- Real Windows toast appears and is plated before content becomes readable;
  current toast signature and provisional geometry constants on supported
  Windows builds.
- In-OBS allowlist, blocklist, picker, panic-hotkey, mode-round-trip, degraded
  chip/mask-all, and wallpaper/sliver exclusion behavior.
- Multi-monitor / mixed-DPI placement and the supported unscaled full-monitor
  Display Capture geometry path.
- 30-minute busy-desktop SRT streaming soak with zero stale events and recorded
  watcher tick p99; two-hour idle memory stability; 60 fps render impact.
- Fresh-machine extraction/install/uninstall UX, security-warning behavior, and
  rendered README link/UI accuracy.

## 7. Final verdict

**VERIFIED.** Clean warnings-as-errors x64 Release build, all automated unit and
watcher tests, clean install, archive structure, no-PDB release hygiene,
artifact identity/version/signature, and corrected public installation claims
all pass for the exact diff fingerprint above. Manual acceptance remains open
and explicitly documented. The sequential spec-guardian gate may now audit this
same diff; any post-report project change invalidates this result.
