# Release verification 007 - allowlist failure-behavior documentation repair

- Run: 2026-07-15 21:45 +08:00, Windows 11 10.0.26200
- Starting HEAD: `7adcf43ed3b8037657463a5efbfdf668e91df1d2`
- Scope: modified `.gitignore` and `README.md`, plus new
  `docs/INSTALLATION.md` and `docs/PUBLISHING_SOP.md`
- Retained history, unchanged: `reports/verify-005.md`,
  `reports/verify-006.md`, and `reports/spec-review-005.md`
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
?? reports/spec-review-005.md
?? reports/verify-005.md
?? reports/verify-006.md
```

The three retained reports were excluded from the candidate fingerprint as
instructed and remained byte-identical throughout this run:

| Retained report | SHA-256 |
|---|---|
| `reports/verify-005.md` | `bd9cb4354c9c0c0b0bac17ca96c3aed4ec2d087753d5dd3671465b3fc00d2028` |
| `reports/verify-006.md` | `ffb8e19746fbbad6a5248e79e37bcd95e2dece0fdf39e7ae7614dbfc67ed2840` |
| `reports/spec-review-005.md` | `ba2c713ccb0617f0b5e01af01a5392ca248ffeb8867aff2cb9da37966730e026` |

Local `master`, `origin/master`, and `git ls-remote origin
refs/heads/master` all resolve to
`7adcf43ed3b8037657463a5efbfdf668e91df1d2`.

Canonical tracked binary-patch fingerprint:

```powershell
cmd /d /c "git diff --binary --no-ext-diff HEAD | git hash-object --stdin"
# 4f1b39453d7f90202f813de14744e3e5a8e260dd
```

Untracked candidate inputs, excluding only the three retained reports and this
new report:

| File | Git blob | SHA-256 |
|---|---|---|
| `docs/INSTALLATION.md` | `c6515ce659ddad958fb58f8f4a0167c90cc549b4` | `9fba784a22d5969b7c14f204af3b965fd67f35b50b732b87841b2fbde7b99726` |
| `docs/PUBLISHING_SOP.md` | `2403948b6b6c455768b4c254368bed10946bda69` | `004b08780afd55eaab90b6259add0e80a4ad95e03f970fccd389f20e82dc0253` |

The next spec guardian must recompute this exact state while excluding the
three retained reports and this verifier report. Any other source or
public-document change invalidates this verification.

## 2. Clean configure and Release build

The ignored `F:\StreamSentry\build_x64` and `F:\StreamSentry\release`
paths were resolved and confirmed to be the two intended workspace children
before both were recursively removed.

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

The frame-decision suite directly exercises the behavior repaired in the
public documentation: stale, missing-snapshot, transition, unresolved-geometry,
mapping-failure, and detection-degraded allowlist states set both `unverified`
and `mask_all`; the corresponding blocklist degradation cases do not set
`mask_all` and retain the source rendering path with the chip.

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
| `streamsentry-0.2.0-windows-x64.zip` | `640a6ddc85151cd20f00b8c4ef3c788c2010f00ff80da2f2db594843590f99d7` |
| source and installed `en-US.ini` | `2684ebd9ff6d56e1464efb4899f12da449e5ee143e95c2dd0b355c0dcf73e6a9` |
| coord-map test executable | `3fb46dfdf3014e1b18f8e772d930d483b2afcdadeed4d2a5e133fb9ecae971f6` |
| plate-gen test executable | `876a4d3a13c0b08aa279cc689bf17170c960df7490944dc2311008a24cea858b` |
| toast-gate test executable | `65a3c702022f45237ed25cefbf0dd8baf56aead5e128221e3a3adb779b4abb86` |
| frame-decide test executable | `a6aefc4360f7736aec0c71c9c39db1ea678a325b64dcaa27a6edae21243b8bbb` |
| watcher self-test executable | `62b3e777fdaa67886cc2ca7f99f23cbdcac934c433032f2d256d8d09bd41e64a` |

The DLL, locale, and test hashes match the prior release verification rounds.
The ZIP hash is expected to vary with fresh archive timestamps.

## 5. spec-review-005 F1 repair

Result: **PASS**.

The repaired public behavior is now stated consistently at every affected
user-facing location:

- README opening: failures are mode-qualified immediately. Blocklist keeps the
  source rendering; allowlist uses its full-source opaque default-deny fallback
  (`README.md:3-8`).
- README behavior explanation: stale heartbeat, watcher death, and unresolved
  geometry show the degradation chip; Blocklist renders the source normally,
  while Allowlist replaces it with an opaque privacy plate
  (`README.md:40-51`).
- README limitation: unsupported Window Capture, scaled/cropped capture, and
  ambiguous identical-resolution monitors are explicitly split into Blocklist
  source-plus-chip and Allowlist plate-plus-chip outcomes
  (`README.md:160-167`).
- Installation SOP: unsupported or ambiguous mapping explicitly keeps the
  source with the chip only in Blocklist, while Allowlist draws the full-source
  opaque plate plus the chip (`docs/INSTALLATION.md:93-106`).

These statements match `SPEC.md:156-160,221-249`, the final fail-open ruling,
`src/frame-decide.c:39-111`, and the rendering branches in
`src/filter.c:521-589`:

- **Blocklist unverified:** render the source unmodified and overlay the opaque
  protection-degraded chip; never guess a mask location.
- **Allowlist unverified:** replace the full source with an opaque privacy plate
  and overlay the protection-degraded chip because the selected mode is
  default-deny.

A case-insensitive search across README, installation/publishing docs,
CHANGELOG, TESTING, SPEC, ARCHITECTURE, AGENTS, and CLAUDE found no remaining
contradictory current public claim. Historical fail-open milestone text is
contextualized by the later explicit allowlist exception in the same durable
documents; current user-facing behavior is always mode-qualified.

## 6. README, installation, package, and UI consistency

Result: **PASS**.

- README and installation SOP name the actual ZIP, exact ProgramData
  self-contained layout, Windows x64/OBS 32.1.2 evidence, unsigned status,
  versioned successful-load log, full-monitor Display Capture limitation, and
  nested-folder check consistently.
- The custom/portable layout correctly maps the DLL to `obs-plugins\64bit` and
  locale data to `data\obs-plugins\streamsentry\locale`.
- The smoke test uses the shipped UI behavior and exact labels. `FilterName`,
  `ModeBlocklist`, `ModeAllowlist`, `PickerAdd`, `PickerRefresh`, and
  `PanicHotkey` agree with `data/locale/en-US.ini` and their wiring in
  `src/filter.c`.
- Update, uninstall, duplicate-copy, checksum, signing, and log troubleshooting
  instructions agree with the exact two-file package.
- `git diff --check` passes.
- An in-memory audit checked all 55 current Markdown files, including retained
  reports, and resolved all 22 local Markdown links and anchors. No missing
  target or bad local anchor was found.

## 7. GitHub publication SOP and workflow

Result: **PASS**.

- Phase headings form the exact uninterrupted sequence 0 through 11, with
  consistent references and order: candidate, verification, sequential gate,
  exact-package acceptance, single scribe, commit/push, tag/draft,
  download-back, visibility, publication, and anonymous check.
- The format job is correctly described as skipped on tag runs and bound to the
  exact commit's preceding `master` push run.
- `0.2.0-beta1` matches the workflow Release-configuration and prerelease-tag
  patterns. The Windows job produces the documented ZIP; the release job
  downloads it, generates SHA-256 notes, and uses `draft: true` plus
  `prerelease: true`.
- The scribe occurs once after VERIFIED + PASS and Phase 4 evidence, touching
  only TESTING and CHANGELOG. Other public-document repairs invalidate the
  gate.
- Commit, push, tag push, visibility, and publication remain separately gated
  by explicit owner authorization. Rollback guidance forbids silent asset
  replacement and unauthorized history rewriting.

No new phase, workflow, package, or authorization contradiction was found.

## 8. Required next gate and manual checks

The sequential spec guardian may now audit this exact fingerprint. Only after
its `PASS` may Phase 4 owner-manual acceptance and the single Phase 5 scribe
proceed.

Still uncovered by this verifier:

- Install the exact final ZIP into the supported ProgramData path, confirm OBS
  32.1.2 loads that path/version, add the filter, and complete the documented
  two-minute blocklist masking/panic smoke test.
- In OBS, explicitly exercise Blocklist unverified source-plus-chip and
  Allowlist unverified full-source-plate-plus-chip behavior and recovery.
- Real Windows toast signature/geometry/timing; allowlist/blocklist picker,
  panic, taskbar, wallpaper/sliver exclusions; mixed-DPI and multi-monitor
  placement.
- 30-minute SRT soak and watcher p99; two-hour idle memory stability; 60 fps
  render impact; fresh-machine unsigned-warning UX.
- Correct stale publication wording in `CHANGELOG.md` through the gated scribe;
  it still says nothing has been pushed even though remote `master` contains
  current HEAD.
- Authorized tag push and Actions success, draft ZIP download-back/checksum and
  OBS retest, public-history/privacy acceptance, visibility change, anonymous
  download/Issues checks, and prerelease publication.

## 9. Final verdict and write boundary

**VERIFIED.** The new candidate passes clean warnings-as-errors Windows x64
Release build, every automated/direct watcher test, exact two-file no-PDB
packaging, artifact identity/version/signature checks, all local Markdown
links/anchors, README/install/package/UI consistency, publication SOP/workflow
order, and the complete spec-review-005 F1 mode-behavior repair. Manual
acceptance remains explicitly open and does not support a final-release claim.

This verifier wrote only `reports/verify-007.md` in the project tree, plus the
explicitly required ignored build/package artifacts under `build_x64` and
`release`. It did not alter `reports/verify-005.md`, `reports/verify-006.md`,
`reports/spec-review-005.md`, source, README, docs, workflows, TESTING,
CHANGELOG, or any other file. It did not stage, commit, push, tag, publish,
change repository visibility, or launch OBS.
