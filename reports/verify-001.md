# Release verification 001 — v0.2.0 beta preparation

- Run: 2026-07-13 23:34 +08:00, Windows 11 10.0.26200
- Starting HEAD: `0be2ec9bfe2956e32fbb38f74c79ae390eed269d`
  (`Add honest AI-authorship disclosure to README`)
- Scope: current uncommitted release-preparation diff, before this report was
  created
- Toolchain: CMake 3.28.0-rc5; Visual Studio 2022 / MSVC 19.44.35224.0;
  Windows SDK 10.0.26100.0; OBS dependency sources 31.1.1
- Configuration: x64 `Release`, `CMAKE_COMPILE_WARNING_AS_ERROR=ON`
- **Verdict: FAILED**

The code, automated tests, clean Release install, and archive hygiene all pass.
The current public README does not yet agree with the supported-platform spec
or the release workflow's checksum delivery, so the complete diff is not ready
for the next gate.

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
```

Tracked diff against HEAD: 9 files, 97 insertions, 53 deletions. Canonical
binary-patch fingerprint (command below):

```powershell
cmd /d /c "git diff --binary --no-ext-diff HEAD | git hash-object --stdin"
# 4aeb3d48c3301771eeff265db18da70c77cc1a9a
```

Untracked input fingerprints:

| File | Git blob | SHA-256 |
|---|---|---|
| `AGENTS.md` | `ee740f9e5256efc0aa6e1a17cd34ff445e4ee724` | `3f876e77bcba0dd79714c55ee335064ff2f38a3bc37184d88db95196e20f80f2` |
| `reports/RULING-2026-07-13-noncontent-window-exclusions.md` | `297afe5f46324ca0f2ea2e837f373449a50081d9` | `b63bb1cdb9f903f082a01e4d8a42b1c937f140b1d2ef801a7e51447e42f6cdc1` |

The next gate must recompute these values while excluding this report. Any
other source/document/workflow change invalidates this evidence.

## 2. Commands and build result

The ignored `build_x64/` and `release/` directories were resolved under
`F:\StreamSentry` and deleted before the run.

The first attempt used the pinned CI configure preset:

```powershell
cmake --preset windows-ci-x64
```

It exited nonzero before generating the project because the existing ignored
`.deps` OBS sub-build cache had been configured for SDK 10.0.26100.0 while the
CI preset requested 10.0.22621.0. CMake reported that the generator platform
did not match. No source compilation occurred. Per the README's documented
local-SDK fallback, `build_x64/` was deleted again and the supported local
preset was configured cleanly:

```powershell
cmake --preset windows-x64-local -DCMAKE_COMPILE_WARNING_AS_ERROR=ON
cmake --build build_x64 --config Release --parallel -- /consoleLoggerParameters:Summary /noLogo
```

Result: **PASS**. Configure exit 0; build exit 0; 0 compiler warnings; 0
compiler errors. CMake cache confirms project version 0.2.0 and warnings as
errors enabled.

Two configure warnings came from the OBS dependency sources, not StreamSentry:

1. `FindDetours.cmake:65`: failed to find Detours version.
2. `win-dshow/virtualcam-module/CMakeLists.txt:14`: empty Virtual Camera GUID.

PowerShell Core (`pwsh`) is not installed on this verifier host. Therefore the
CI-only `Build-Windows.ps1` / `Package-Windows.ps1` wrappers, which require
PowerShell 7.2+, could not be invoked byte-for-byte. Their underlying CMake
install and `Compress-Archive` operations were executed directly below. No
dependency was installed.

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

The watcher self-test passed every deterministic leg: start/heartbeat,
Notepad blocklist appearance and disappearance, deduplicated named picker
enumeration (7 processes), allowlist masking, switch back to blocklist,
debug-kill stale-heartbeat degradation behavior, heartbeat recovery, and clean
shutdown. It left zero Notepad processes behind.

The toast leg remained **INCONCLUSIVE** because no banner was displayed
(consistent with the documented Do Not Disturb / Focus Assist condition).
Real-toast timing remains a manual release check; it did not make the
deterministic self-test fail.

Local `clang-format` and `gersemi` executables are absent, and the repository's
format actions are Linux/macOS-only, so format-workflow parity was not run on
this Windows verifier host.

## 4. Clean install and package dry run

Commands (equivalent underlying operations of `Package-Windows.ps1`):

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

Archive result: **PASS**. One root folder (`streamsentry`), exactly two file
entries:

```text
streamsentry\bin\64bit\streamsentry.dll  77312 bytes
streamsentry\data\locale\en-US.ini        1276 bytes
```

PDB hygiene: installed PDB count 0; archive PDB count 0. The installed DLL is
byte-identical to the built Release DLL. FileVersion/ProductVersion are 0.2.0;
ProductName is `streamsentry`; Authenticode status is `NotSigned`, matching the
README's beta warning.

Artifact SHA-256 values:

| Artifact | SHA-256 |
|---|---|
| Release DLL (built and installed) | `3ac9946d2fa4ef53b2aba6af10e7c31b2a9257c8bdea9e72e398905f799c24d4` |
| `streamsentry-0.2.0-windows-x64.zip` | `b5c1e4a47f138e27771cc9cb10e68d8f92988435d9ae1578cc87fe9ca5ef0b55` |
| installed `en-US.ini` | `65757c15e4c77c7d21d0da0049d7528226e094d13e64634ce17e276a1bdf2999` |
| coord-map test executable | `3fb46dfdf3014e1b18f8e772d930d483b2afcdadeed4d2a5e133fb9ecae971f6` |
| plate-gen test executable | `876a4d3a13c0b08aa279cc689bf17170c960df7490944dc2311008a24cea858b` |
| toast-gate test executable | `65a3c702022f45237ed25cefbf0dd8baf56aead5e128221e3a3adb779b4abb86` |
| frame-decide test executable | `a6aefc4360f7736aec0c71c9c39db1ea678a325b64dcaa27a6edae21243b8bbb` |
| watcher self-test executable | `62b3e777fdaa67886cc2ca7f99f23cbdcac934c433032f2d256d8d09bd41e64a` |

## 5. README installation validation

Validated claims that **match**:

- The package name is `streamsentry-0.2.0-windows-x64.zip`.
- Its root/layout exactly matches
  `streamsentry/bin/64bit/streamsentry.dll` plus
  `streamsentry/data/locale/en-US.ini`; no nested duplicate plugin folder.
- `C:\ProgramData\obs-studio\plugins` and this bundle layout match the
  official OBS Plugins Guide's recommended Windows manual-install structure.
- Source contains the exact log payload
  `plugin loaded successfully (version 0.2.0)` (OBS prefixes it with
  `[streamsentry]`).
- Local OBS logs show OBS 32.1.2 x64 loading StreamSentry 0.2.0 successfully on
  2026-07-10.
- The built DLL is unsigned, as disclosed.

Release-blocking mismatches:

1. **`CHECKSUMS.txt` is not a downloadable release asset.** README lines 66–68
   instruct users to download both the zip and `CHECKSUMS.txt`. The tag workflow
   creates `CHECKSUMS.txt`, but uses it only as `body_path`; the release `files:`
   list uploads `*.zip`, `*.exe`, `*.pkg`, `*.deb`, `*.ddeb`, and `*.tar.xz`, not
   `CHECKSUMS.txt`. A user will see checksum text in the release body but cannot
   download the named file as instructed. Either upload the file or change the
   README to say the SHA-256 is in the release notes.
2. **Windows 10 support is broader than the locked spec.** README line 60 says
   `Windows 10 or Windows 11, x64`; SPEC Part 1 limits support to Windows 10
   21H2+ / Windows 11. Restore `Windows 10 21H2+` in the public requirements.

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
- Fresh-machine extraction/install/uninstall UX, security warning behavior, and
  rendered README link/UI accuracy.

## 7. Final verdict

**FAILED.** Build, all available automated tests, clean Release install, archive
layout, Release PDB exclusion, version metadata, and DLL identity are verified.
The hub must correct the two public README/release-workflow mismatches above and
request a fresh sequential verifier run. Any post-report change invalidates this
report; the spec-guardian must not treat this FAILED report as a passing gate.
