# Release verification 005 - publication-readiness SOP candidate

- Run: 2026-07-15 21:28 +08:00, Windows 11 10.0.26200
- Starting HEAD: `7adcf43ed3b8037657463a5efbfdf668e91df1d2`
- Scope: modified `.gitignore` and `README.md`, plus new
  `docs/INSTALLATION.md` and `docs/PUBLISHING_SOP.md`
- Toolchain: CMake 3.28.0-rc5; Visual Studio 2022 17.14.28 / MSVC
  19.44.35224.0; Windows SDK 10.0.26100.0; OBS dependency sources 31.1.1
- Configuration: Windows x64 `Release`,
  `CMAKE_COMPILE_WARNING_AS_ERROR=ON`
- **Verdict: FAILED**

The code, tests, two-file package, installation instructions, Markdown links,
and most publishing commands pass. The publishing SOP has two process errors
that must be repaired and reverified before this documentation candidate is
used for a public beta.

## 1. Freshness fingerprint

Starting and final non-ignored project status, before this report was created:

```text
 M .gitignore
 M README.md
?? docs/INSTALLATION.md
?? docs/PUBLISHING_SOP.md
```

Local `master`, `origin/master`, and `git ls-remote origin
refs/heads/master` all resolve to
`7adcf43ed3b8037657463a5efbfdf668e91df1d2`.

Tracked diff: 2 files, 48 insertions, 24 deletions. Canonical binary-patch
fingerprint:

```powershell
cmd /d /c "git diff --binary --no-ext-diff HEAD | git hash-object --stdin"
# 4057c4fcbc2f60de8dc0268b8a0a4c82142e1e21
```

Untracked candidate inputs:

| File | Git blob | SHA-256 |
|---|---|---|
| `docs/INSTALLATION.md` | `48aee785bcf94a75643e06236c261b1700e505a6` | `d05bb664e8aa8ca56b6fdcdcc0130316556cf887dc62bd6b6827b0a6421512f2` |
| `docs/PUBLISHING_SOP.md` | `925c01dcf562e318706f693c66d80ba57995ae0b` | `6b058d27da41e0b65b661fe8bf1155af98309aa8abbe636f95dbb94036eb0b45` |

The next verifier must recompute the complete candidate while excluding only
this retained failed report and its own new report. Any documentation repair
changes this fingerprint.

## 2. Clean configure and Release build

The ignored `F:\StreamSentry\build_x64` and `F:\StreamSentry\release`
paths were resolved and confirmed to be the two intended children of the
workspace before both were recursively removed.

```powershell
cmake --preset windows-x64-local -DCMAKE_COMPILE_WARNING_AS_ERROR=ON
cmake --build build_x64 --config Release --parallel -- /consoleLoggerParameters:Summary /noLogo
```

Result: **PASS**. Clean configure and build exited 0; the build reported 0
warnings and 0 errors. The cache confirms project version 0.2.0, Visual Studio
2022 x64 generation, and warnings-as-errors enabled. Configure selected Windows
SDK 10.0.26100.0.

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
| CTest aggregate | PASS - 4/4, 0 failed, 0.18 s |
| coord-map direct | PASS, exit 0 |
| plate-gen direct | PASS, exit 0 |
| toast-gate direct | PASS, exit 0 |
| frame-decide direct | PASS, exit 0 |
| watcher self-test | PASS, exit 0 |

The watcher self-test passed heartbeat, Notepad blocklist appearance and
disappearance, deduplicated named picker enumeration (11 processes), allowlist
masking, blocklist restoration, debug-kill degradation, recovery, and clean
shutdown. It left zero Notepad processes. The real-toast leg remained
**INCONCLUSIVE** because no banner appeared, consistent with the documented Do
Not Disturb / Focus Assist condition.

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
| `streamsentry-0.2.0-windows-x64.zip` | `7e179bf8469d2629e660c6ef9cbdbc35fcaa482f8d1933122d5baa5fb95851dc` |
| source and installed `en-US.ini` | `2684ebd9ff6d56e1464efb4899f12da449e5ee143e95c2dd0b355c0dcf73e6a9` |
| coord-map test executable | `3fb46dfdf3014e1b18f8e772d930d483b2afcdadeed4d2a5e133fb9ecae971f6` |
| plate-gen test executable | `876a4d3a13c0b08aa279cc689bf17170c960df7490944dc2311008a24cea858b` |
| toast-gate test executable | `65a3c702022f45237ed25cefbf0dd8baf56aead5e128221e3a3adb779b4abb86` |
| frame-decide test executable | `a6aefc4360f7736aec0c71c9c39db1ea678a325b64dcaa27a6edae21243b8bbb` |
| watcher self-test executable | `62b3e777fdaa67886cc2ca7f99f23cbdcac934c433032f2d256d8d09bd41e64a` |

The DLL, locale, and test hashes match the previous verified Release build; the
new ZIP hash reflects fresh archive timestamps.

## 5. Installation, README, package, and UI consistency

Result: **PASS**.

- The README and installation SOP name the actual ZIP, exact ProgramData
  self-contained plugin layout, Windows x64/OBS 32.1.2 evidence, unsigned
  status, versioned successful-load log, full-monitor Display Capture
  limitation, and accidental nested-folder check consistently.
- The standard package mapping is exact. The documented traditional
  custom/portable mapping correctly separates the DLL into
  `obs-plugins\64bit` and locale data into
  `data\obs-plugins\streamsentry\locale`.
- The two-minute smoke test uses actual UI labels and behavior. `FilterName`,
  `PickerAdd`, `PickerRefresh`, `ModeBlocklist`, and `PanicHotkey` agree with
  `data/locale/en-US.ini` and their wiring in `src/filter.c`.
- The update/uninstall and duplicate-copy warnings agree with both supported
  package layouts and do not claim that copied files alone prove protection.
- `git diff --check` passes.
- An in-memory audit checked 52 Markdown files and resolved all 22 local
  Markdown links and Markdown anchors; no missing target or bad local anchor was
  found.

## 6. Publishing workflow checks that pass

- The read-only Phase 1 Git commands execute successfully, and local/remote
  `master` currently agree.
- `0.2.0-beta1` matches both the Release configuration test in
  `.github/workflows/build-project.yaml` and the prerelease-tag pattern in
  `.github/workflows/push.yaml`.
- A recognized tag runs the Windows job in Release configuration, installs the
  plugin, creates `streamsentry-0.2.0-windows-x64.zip`, uploads it, downloads it
  into the release job, generates a SHA-256 entry, and invokes the pinned
  release action with `draft: true` and `prerelease: true`.
- macOS and Ubuntu build jobs are intentionally disabled, so the SOP correctly
  treats the Windows ZIP as the binary beta surface.
- Commit, push, tag push, visibility change, and Release publication are all
  separated behind explicit owner authorization.
- The download-back, anonymous check, and do-not-silently-replace rollback
  instructions are appropriate for the actual draft-release flow.

## 7. Blocking findings

### F1 - Tag-run format status cannot be green

`docs/PUBLISHING_SOP.md:146-150` tells the maintainer to open the tag-triggered
Push run and confirm the format and Windows build jobs are green. In
`.github/workflows/push.yaml:14-17`, `check-format` runs only when
`github.ref_name` is `master` or `main`; on `0.2.0-beta1` it is skipped, not
green. A maintainer following the SOP cannot satisfy the stated gate.

Repair the SOP to distinguish the already-green branch format run from the tag
run, and on the tag run require the actual jobs that execute (including the
Windows build and draft-release job) to succeed.

### F2 - The scribe/manual-acceptance order is internally inconsistent

`docs/PUBLISHING_SOP.md:83-98` places the scribe update inside Phase 3, directly
after verifier + guardian. Phase 4 then runs the exact ZIP in OBS and
`docs/PUBLISHING_SOP.md:105-110` asks the gated scribe to record that new result
in `TESTING.md`. This either requires an undocumented second scribe edit after
the supposed Phase 3 freeze, or produces a Phase 3 TESTING entry that cannot
contain the later Phase 4 evidence. It also conflicts with the adjacent rule
that post-fingerprint public-document changes require freeze/rerun handling.

Repair the order so exact-package manual acceptance occurs after VERIFIED +
PASS but before the single scribe update, or explicitly define the second gate
required after the later documentation edit. The first option matches the
project's intended verifier -> guardian -> scribe gate while allowing the
scribe to record all evidence once.

### Required scribe correction before publication

`CHANGELOG.md:10` still says nothing has been pushed or tagged, while
`origin/master` already equals current HEAD. This file is outside the current
hub edit scope and the established workflow reserves its update for the scribe,
so it is not counted as a third verifier finding. It remains a mandatory scribe
correction before commit/tag/publication, exactly as Phase 1 requires.

## 8. Manual checks still uncovered

- Install the exact final ZIP into the supported ProgramData path, confirm OBS
  32.1.2 loads that path/version, add the filter, and complete the documented
  two-minute masking/panic smoke test.
- Download back the GitHub Actions draft-release ZIP, validate its checksum and
  exact two-file layout, then repeat the OBS smoke test.
- Real Windows toast signature/geometry/timing; in-OBS allowlist, blocklist,
  picker, panic, degraded chip/mask-all, wallpaper/sliver behavior; mixed-DPI
  and multi-monitor placement.
- 30-minute SRT soak and watcher p99; two-hour idle memory stability; 60 fps
  render impact; fresh-machine security-warning UX.
- History/private-data acceptance, repository visibility change, anonymous
  download/Issues checks, and actual prerelease publication.

## 9. Final verdict and write boundary

**FAILED.** Clean build, tests, packaging, hashes, version/signature, local
links, installation SOP, and most of the publishing workflow are sound. The
two publishing-SOP process errors in section 7 must be repaired, after which a
fresh verifier must bind the new documentation fingerprint. No spec guardian
should audit this failed fingerprint.

This verifier wrote only `reports/verify-005.md` in the project tree, plus the
explicitly required ignored build/package artifacts under `build_x64` and
`release`. It did not edit source, README, docs, workflows, TESTING, CHANGELOG,
or an existing report. It did not stage, commit, push, tag, publish, change
repository visibility, or launch OBS.
