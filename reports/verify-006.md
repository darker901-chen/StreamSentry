# Release verification 006 - repaired publication-readiness SOP

- Run: 2026-07-15 21:34 +08:00, Windows 11 10.0.26200
- Starting HEAD: `7adcf43ed3b8037657463a5efbfdf668e91df1d2`
- Scope: modified `.gitignore` and `README.md`, plus new
  `docs/INSTALLATION.md` and repaired `docs/PUBLISHING_SOP.md`
- Retained failed history: `reports/verify-005.md`, unchanged
- Toolchain: CMake 3.28.0-rc5; Visual Studio 2022 17.14.28 / MSVC
  19.44.35224.0; Windows SDK 10.0.26100.0; OBS dependency sources 31.1.1
- Configuration: Windows x64 `Release`,
  `CMAKE_COMPILE_WARNING_AS_ERROR=ON`
- **Verdict: VERIFIED**

## 1. Freshness fingerprint

Starting and final non-ignored project status, before this report was created:

```text
 M .gitignore
 M README.md
?? docs/INSTALLATION.md
?? docs/PUBLISHING_SOP.md
?? reports/verify-005.md
```

`reports/verify-005.md` is retained failed-gate history and was excluded from
the candidate fingerprint as instructed. It remained byte-identical throughout
this run, SHA-256
`bd9cb4354c9c0c0b0bac17ca96c3aed4ec2d087753d5dd3671465b3fc00d2028`.

Local `master`, `origin/master`, and `git ls-remote origin
refs/heads/master` all resolve to
`7adcf43ed3b8037657463a5efbfdf668e91df1d2`.

Tracked diff: 2 files, 48 insertions, 24 deletions. Canonical binary-patch
fingerprint:

```powershell
cmd /d /c "git diff --binary --no-ext-diff HEAD | git hash-object --stdin"
# 4057c4fcbc2f60de8dc0268b8a0a4c82142e1e21
```

Untracked candidate inputs, excluding only retained `verify-005` and this new
report:

| File | Git blob | SHA-256 |
|---|---|---|
| `docs/INSTALLATION.md` | `48aee785bcf94a75643e06236c261b1700e505a6` | `d05bb664e8aa8ca56b6fdcdcc0130316556cf887dc62bd6b6827b0a6421512f2` |
| `docs/PUBLISHING_SOP.md` | `2403948b6b6c455768b4c254368bed10946bda69` | `004b08780afd55eaab90b6259add0e80a4ad95e03f970fccd389f20e82dc0253` |

The next spec guardian must recompute this exact state while excluding the two
verifier reports. Any other source or public-document change invalidates this
verification.

## 2. Clean configure and Release build

The ignored `F:\StreamSentry\build_x64` and `F:\StreamSentry\release`
paths were resolved and confirmed to be the two intended children of the
workspace before both were recursively removed.

```powershell
cmake --preset windows-x64-local -DCMAKE_COMPILE_WARNING_AS_ERROR=ON
cmake --build build_x64 --config Release --parallel -- /consoleLoggerParameters:Summary /noLogo
```

Result: **PASS**. Clean configure and Release build exited 0. The StreamSentry
build reported 0 warnings and 0 errors. The cache confirms project version
0.2.0, Visual Studio 2022 x64 generation, and warnings-as-errors enabled.
Configure selected Windows SDK 10.0.26100.0.

The two configure warnings are from OBS dependency sources, not StreamSentry:

1. `FindDetours.cmake:65`: failed to find the Detours version.
2. `win-dshow/virtualcam-module/CMakeLists.txt:14`: empty Virtual Camera GUID.

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
| CTest aggregate | PASS - 4/4, 0 failed, 0.17 s |
| coord-map direct | PASS, exit 0 |
| plate-gen direct | PASS, exit 0 |
| toast-gate direct | PASS, exit 0 |
| frame-decide direct | PASS, exit 0 |
| watcher self-test | PASS, exit 0 |

The watcher self-test passed heartbeat, Notepad blocklist appearance and
disappearance, deduplicated named picker enumeration (10 processes), allowlist
masking, blocklist restoration, debug-kill degradation, recovery, and clean
shutdown. It left zero Notepad processes.

The real-toast leg remained **INCONCLUSIVE** because no banner appeared,
consistent with the documented Do Not Disturb / Focus Assist condition. Real
toast timing remains owner-manual.

## 4. Clean install and exact package

```powershell
cmake --install build_x64 --prefix "$PWD\release\Release" --config Release
Compress-Archive -Path (Get-ChildItem release\Release).FullName `
  -DestinationPath release\streamsentry-0.2.0-windows-x64.zip `
  -CompressionLevel Optimal -Force
```

Result: **PASS**. The install tree and archive each contain exactly:

```text
streamsentry/bin/64bit/streamsentry.dll  77312 bytes
streamsentry/data/locale/en-US.ini        1429 bytes
```

There is one `streamsentry` root, no extra file, and no PDB in either the
install tree or ZIP. The installed DLL is byte-identical to the built DLL; the
installed locale is byte-identical to the source locale. DLL FileVersion and
ProductVersion are 0.2.0, ProductName is `streamsentry`, and Authenticode status
is `NotSigned`.

| Artifact | SHA-256 |
|---|---|
| Release DLL (built and installed) | `3ac9946d2fa4ef53b2aba6af10e7c31b2a9257c8bdea9e72e398905f799c24d4` |
| `streamsentry-0.2.0-windows-x64.zip` | `c64590bbab6ef67790edef4bc3291b1580871817adeae2a319047afd2ca80d1d` |
| source and installed `en-US.ini` | `2684ebd9ff6d56e1464efb4899f12da449e5ee143e95c2dd0b355c0dcf73e6a9` |
| coord-map test executable | `3fb46dfdf3014e1b18f8e772d930d483b2afcdadeed4d2a5e133fb9ecae971f6` |
| plate-gen test executable | `876a4d3a13c0b08aa279cc689bf17170c960df7490944dc2311008a24cea858b` |
| toast-gate test executable | `65a3c702022f45237ed25cefbf0dd8baf56aead5e128221e3a3adb779b4abb86` |
| frame-decide test executable | `a6aefc4360f7736aec0c71c9c39db1ea678a325b64dcaa27a6edae21243b8bbb` |
| watcher self-test executable | `62b3e777fdaa67886cc2ca7f99f23cbdcac934c433032f2d256d8d09bd41e64a` |

The DLL, locale, and test hashes match verify-005 and the previous verified
Release build. The ZIP hash is expected to vary with fresh archive timestamps.

## 5. README, installation, package, and UI consistency

Result: **PASS**.

- README and installation SOP name the actual ZIP, exact ProgramData
  self-contained plugin layout, Windows x64/OBS 32.1.2 evidence, unsigned
  status, versioned successful-load log, full-monitor Display Capture
  limitation, and nested-folder check consistently.
- The custom/portable traditional layout correctly separates the DLL into
  `obs-plugins\64bit` and locale data into
  `data\obs-plugins\streamsentry\locale`.
- The smoke test uses actual UI behavior and exact shipped labels:
  `FilterName`, `PickerAdd`, `PickerRefresh`, `ModeBlocklist`, and
  `PanicHotkey` agree with `data/locale/en-US.ini` and `src/filter.c`.
- Update, uninstall, duplicate-copy, checksum, and log-based troubleshooting
  instructions agree with the package and do not claim copied files alone prove
  protection.
- `git diff --check` passes.
- An in-memory audit checked all 53 current Markdown files, including retained
  verify-005, and resolved all 22 local Markdown links and anchors. No missing
  target or bad local anchor was found.

## 6. verify-005 blocker resolution and phase audit

Result: **PASS**.

### F1 - tag-run format status

Resolved. `docs/PUBLISHING_SOP.md:166-175` now requires the Windows build and
draft-release jobs to succeed on the tag run, explicitly says the format job is
intentionally skipped for tag runs, and directs the maintainer to confirm
format on the exact commit's preceding `master` push run. This matches
`.github/workflows/push.yaml:14-17,28-32`.

### F2 - scribe/manual-acceptance order

Resolved. Phase 3 now freezes the VERIFIED + PASS candidate for Phase 4 exact
package acceptance, then invokes one scribe in Phase 5. Phase 4 preserves its
manual result as input; Phase 5 records machine and manual evidence exactly
once in TESTING and CHANGELOG. The exception is narrowly bounded to those two
scribe-owned files, and any other repair returns to the hub and reruns the gate.

### Complete order and workflow behavior

- Phase headings are present exactly once and form the uninterrupted sequence
  0 through 11.
- All forward references to Phases 4 and 5 agree with the actual headings and
  ordering; commit/push, tag/draft, download-back, visibility, publication, and
  anonymous checks then advance in order through Phase 11.
- `0.2.0-beta1` matches the workflow Release-configuration and prerelease-tag
  patterns. The Windows job produces the documented ZIP; the release job
  downloads it, generates SHA-256 notes, and uses `draft: true` plus
  `prerelease: true`.
- The SOP correctly treats the disabled macOS/Ubuntu jobs as outside this
  Windows beta surface and does not require an impossible job state.
- External mutations remain separately gated by explicit owner authorization.

No new phase-number, ordering, tag-run, packaging, or authorization
contradiction was found.

## 7. Required next gate and manual checks

The sequential spec guardian may now audit this exact fingerprint. Only after
its `PASS` may the Phase 4 owner-manual exact-package acceptance and Phase 5
scribe proceed.

Still uncovered by this verifier:

- Install the exact final ZIP into the supported ProgramData path, confirm OBS
  32.1.2 loads that path/version, add the filter, and complete the two-minute
  masking/panic smoke test.
- Real Windows toast signature/geometry/timing; in-OBS allowlist, blocklist,
  picker, panic, degraded chip/mask-all, wallpaper/sliver behavior; mixed-DPI
  and multi-monitor placement.
- 30-minute SRT soak and watcher p99; two-hour idle memory stability; 60 fps
  render impact; fresh-machine security-warning UX.
- Correct stale publication wording in `CHANGELOG.md` through the single gated
  scribe update; `CHANGELOG.md:10` currently says nothing has been pushed even
  though `origin/master` already contains current HEAD.
- Tag/push and GitHub Actions success, draft-release asset download-back and
  checksum validation, repository history/privacy acceptance, public visibility
  change, anonymous download/Issues checks, and prerelease publication.

## 8. Final verdict and write boundary

**VERIFIED.** The repaired candidate passes clean warnings-as-errors Windows
x64 Release build, all automated/direct watcher tests, exact two-file no-PDB
packaging, artifact identity/version/signature checks, local link/anchor checks,
README/install/UI consistency, publishing-workflow audit, both verify-005
repairs, and complete phase-order validation. Manual acceptance remains
explicitly open and does not support a final-release claim.

This verifier wrote only `reports/verify-006.md` in the project tree, plus the
explicitly required ignored build/package artifacts under `build_x64` and
`release`. It did not alter retained `reports/verify-005.md`, source, README,
docs, workflows, TESTING, CHANGELOG, or any other existing report. It did not
stage, commit, push, tag, publish, change repository visibility, or launch OBS.
