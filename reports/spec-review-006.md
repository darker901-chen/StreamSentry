# Release spec review 006 — repaired mode-specific failure wording

- Run: 2026-07-15 21:48 +08:00
- Starting HEAD: `7adcf43ed3b8037657463a5efbfdf668e91df1d2`
- Matching verifier: `reports/verify-007.md` (`VERIFIED`)
- Scope: modified `.gitignore` and `README.md`, plus new
  `docs/INSTALLATION.md` and `docs/PUBLISHING_SOP.md`; retained failed and
  verifier history through `reports/spec-review-005.md` and
  `reports/verify-007.md`
- **Verdict: PASS**

The exact newly verified fingerprint conforms to the binding v0.2 rules. The
`spec-review-005` blocker is resolved at every affected user entry point:
Blocklist unverified states preserve the source and show the opaque degradation
chip, while Allowlist unverified states replace the source with the full-source
opaque privacy plate and stack the chip because that selected mode is
default-deny. No remaining current public behavior text contradicts that mode
exception.

## 1. Freshness and verifier match

Current `HEAD`, local `master`, `origin/master`, and the remote `master` ref all
resolve to `7adcf43ed3b8037657463a5efbfdf668e91df1d2`.

Recomputed immediately before this report while excluding retained
`reports/verify-005.md`, `reports/verify-006.md`,
`reports/spec-review-005.md`, matching `reports/verify-007.md`, and this new
report:

```text
Tracked diff against HEAD: 2 files, 66 insertions, 39 deletions
git diff --binary --no-ext-diff HEAD | git hash-object --stdin
4f1b39453d7f90202f813de14744e3e5a8e260dd
```

| Untracked candidate input | Git blob | SHA-256 | Match |
|---|---|---|---|
| `docs/INSTALLATION.md` | `c6515ce659ddad958fb58f8f4a0167c90cc549b4` | `9fba784a22d5969b7c14f204af3b965fd67f35b50b732b87841b2fbde7b99726` | PASS |
| `docs/PUBLISHING_SOP.md` | `2403948b6b6c455768b4c254368bed10946bda69` | `004b08780afd55eaab90b6259add0e80a4ad95e03f970fccd389f20e82dc0253` | PASS |

All retained reports remain byte-identical to the matching verifier's record:

| Excluded report | SHA-256 | Match |
|---|---|---|
| `reports/verify-005.md` | `bd9cb4354c9c0c0b0bac17ca96c3aed4ec2d087753d5dd3671465b3fc00d2028` | PASS |
| `reports/verify-006.md` | `ffb8e19746fbbad6a5248e79e37bcd95e2dece0fdf39e7ae7614dbfc67ed2840` | PASS |
| `reports/spec-review-005.md` | `ba2c713ccb0617f0b5e01af01a5392ca248ffeb8867aff2cb9da37966730e026` | PASS |

The matching `verify-007.md` is unchanged (Git blob
`b59dc6fdc380acbf923e9a177cae1e208bd0bd6e`, SHA-256
`b82d74b3e31946a6307a6bb7939aea9de5aa233cb6271ab510426e4fac937238`).
`git diff --check` passes. No candidate, retained-report, source, workflow, or
public-document drift occurred before this report was written.

## 2. `spec-review-005` F1 resolution

**PASS.** The repaired public wording now states the same mode-specific failure
behavior in all four affected locations:

- The README opening says the status chip reports unverified protection,
  Blocklist keeps the source rendering, and Allowlist uses its full-source
  opaque default-deny fallback (`README.md:3-8`).
- The behavior explanation applies stale heartbeat, watcher death, and
  unresolved geometry to both modes, then explicitly separates Blocklist
  source rendering from Allowlist's entire-source opaque privacy plate, with
  the non-disableable chip present until recovery (`README.md:40-51`).
- The limitations section gives unsupported Window Capture, scaled/cropped
  capture, and ambiguous identical-resolution monitors the Blocklist
  source-plus-chip outcome and the Allowlist full-source-plate-plus-chip
  outcome (`README.md:159-169`).
- The installation SOP repeats the same split immediately after selecting the
  first-test mode: an unmappable mask keeps the source in Blocklist, while
  Allowlist draws the full opaque privacy plate; both show the degradation chip
  (`docs/INSTALLATION.md:93-106`).

This is the binding and implemented behavior:

- `SPEC.md:221-242` requires confident-only placement, Blocklist unmodified
  rendering plus chip/log on unverified protection, and the explicit Allowlist
  mask-all exception.
- The final owner ruling and its M7 addendum require every unverified Allowlist
  reason, including `detection_degraded`, to fall toward mask-all, while
  Blocklist remains fail-open with visible notice
  (`reports/RULING-2026-07-09-fail-open.md:27-40,82-93`).
- `src/frame-decide.c:39-111` sets `mask_all` on missing/stale/transition,
  detection-degraded, unresolved-geometry, and invalid-mapping Allowlist paths
  but not the corresponding Blocklist paths. `src/filter.c:521-537` draws the
  full plate and chip before composing the target; the later Blocklist path
  retains the target and draws the chip (`src/filter.c:544-589`).
- `verify-007.md` reports direct frame-decision tests for all repaired failure
  cases and a clean Release build with all suites passing.

### Current-text contradiction sweep

A case-insensitive audit covered README, installation and publishing SOPs,
CHANGELOG, TESTING, SPEC, ARCHITECTURE, AGENTS, and CLAUDE for normal/unmodified
rendering, degraded/unverified states, blackout, default-deny, and mask-all.
No unresolved contradiction remains:

- README and installation text are now always mode-qualified where the failure
  output is described.
- `SPEC.md:224-242`, `AGENTS.md:7`, and `CLAUDE.md:7` put the Allowlist exception
  directly beside the general fail-open rule.
- `ARCHITECTURE.md:154-180` describes the general degraded path, then explicitly
  defines every Allowlist-unverified reason as plate-instead-of-target with the
  chip on top.
- CHANGELOG and TESTING preserve the historical M6.5 Blocklist-era fail-open
  wording, but their later M7 sections explicitly supersede it for Allowlist
  and retain it for Blocklist (`CHANGELOG.md:175-187,259-320`;
  `TESTING.md:641-665,840-882`). They are chronological evidence, not current
  contrary instructions.

## 3. Installation, package, and UI audit

| Rule | Result | Evidence |
|---|---|---|
| Correct download | PASS | README and installation SOP name the actual `streamsentry-0.2.0-windows-x64.zip` Release asset, distinguish it from GitHub source archives, require SHA-256 comparison, and disclose the unsigned beta (`README.md:63-99`; `docs/INSTALLATION.md:7-23`). |
| Standard layout | PASS | The user copies one `streamsentry` root under `%ProgramData%\obs-studio\plugins`; exact DLL and locale paths are stated, and nested `streamsentry\streamsentry` is diagnosed (`README.md:70-81`; `docs/INSTALLATION.md:30-48`). This matches the verified archive. |
| Custom/portable layout | PASS | DLL and locale are separately mapped to `<OBS folder>\obs-plugins\64bit` and `<OBS folder>\data\obs-plugins\streamsentry\locale`; the log determines the loaded copy and duplicate versions are prohibited (`docs/INSTALLATION.md:50-73`). |
| Load confirmation | PASS | Windows x64, OBS 32.1.2 evidence, version `0.2.0`, the successful-load log, English locale fallback, and stop-on-missing-log wording match the package and runtime (`README.md:60-62,92-117`; `docs/INSTALLATION.md:75-91,155-168`). |
| First-run smoke test | PASS | **StreamSentry**, **Masking mode**, **Blocklist**, **Open windows**, **Add to list**, **Refresh list**, and **StreamSentry: mask everything (panic)** agree with `data/locale/en-US.ini:1-16` and their `src/filter.c` wiring. The test narrowly proves loading, picker-to-blocklist routing, opaque plate drawing, and panic on that installation and openly excludes broader acceptance (`docs/INSTALLATION.md:93-127`). |
| Update/uninstall/troubleshooting | PASS | The SOP closes OBS, removes the path the log actually loaded, prevents duplicate copies, verifies the replacement version, covers both supported layouts, and asks users to redact private log content (`docs/INSTALLATION.md:129-195`). |
| Version, exact package, and signature | PASS | `buildspec.json` is `0.2.0`; Release installation targets `streamsentry/bin/64bit` and `streamsentry/data`, while the PDB is limited to non-Release configurations (`cmake/windows/helpers.cmake:23-32,62-78`). `verify-007.md` proves the exact two-file no-PDB archive, byte identity, version resources, unsigned status, and hashes. |

## 4. Remaining binding-rule audit

| Rule | Result | Evidence |
|---|---|---|
| Privacy assist / confidence | PASS | Current user docs do not claim insurance or universal protection. They identify manual gaps, unsupported geometry, deterministic rules, chip/log reporting, and the mode-specific failure direction (`README.md:3-14,34-57,159-216`; `docs/INSTALLATION.md:3-5,89-127`). |
| Opaque only | PASS | README prohibits blur and pixelation and describes only notification cards and privacy plates (`README.md:53-57`). The unchanged renderer uses alpha-1 solid fallback whenever a confident plate texture is unavailable (`src/filter.c:569-584`). |
| Deterministic only | PASS | README states Win32 enumeration plus UI Automation, no runtime AI inference, and no captured-content service (`README.md:34-38`). The documentation-only repair adds no detection or network path. |
| Non-content exclusions / allowlist semantics | PASS | README says Allowlist masks unapproved content-bearing application/taskbar windows while known wallpaper hosts remain visible; separate lists, empty Allowlist, toast/password guards, picker, and panic remain accurately described (`README.md:126-157`). This matches SPEC and the final non-content ruling. |
| v0.2 scope | PASS | No source or workflow code changed. The candidate adds installation/publication guidance only and introduces none of macOS support, AI detection, blur, per-app policy, Focus Assist integration, tray icon, auto-update, or another backlog item. Dormant macOS/Linux template jobs remain outside the Windows beta surface. |
| Dependencies | PASS | The plugin still links libobs plus Windows SDK system libraries only; optional frontend API and Qt stay OFF (`CMakeLists.txt:7-26,62`). Documentation adds no dependency. |
| GPL-2.0-or-later / origin | PASS | README retains GPL-2.0-or-later and original-project provenance, and the publishing SOP makes license acceptance a pre-publication check (`README.md:279-293`; `docs/PUBLISHING_SOP.md:17-33`). The source repository is made public before binary publication, leaving tagged corresponding source available with the prerelease. |
| Status honesty | PASS | README labels the build `v0.2.0 beta`, says machine-verified but preserves real-device checks as manual, and does not claim a tag, public repository, or published Release already exists (`README.md:10-14`). |

## 5. Publishing SOP and GitHub workflow

**PASS.** The complete Phase sequence remains 0 through 11 exactly once:
candidate → verification → sequential gate → exact-package acceptance → one
scribe update → commit/push → tag/draft → download-back → visibility → Release
publication → anonymous check.

- The repaired `verify-005` F1 remains resolved: the tag run requires the
  Windows build and draft-release jobs, while format is intentionally skipped
  and checked on the exact commit's preceding `master` push
  (`docs/PUBLISHING_SOP.md:166-175`; `.github/workflows/push.yaml:14-32`).
- The repaired `verify-005` F2 remains resolved: Phase 4 exact-package evidence
  precedes the single Phase 5 scribe update, which is restricted to TESTING and
  CHANGELOG (`docs/PUBLISHING_SOP.md:83-133`).
- `0.2.0-beta1` matches the Release-configuration and prerelease tag patterns.
  The Windows job packages/uploads the documented artifact; the release job
  downloads artifacts, generates SHA-256 notes, and creates a draft prerelease
  (`.github/workflows/build-project.yaml:43-48,257-302`;
  `.github/workflows/push.yaml:44-119`).
- Download-back verifies checksum, exact layout, absence of PDB, versioned load,
  and smoke behavior before visibility/publication
  (`docs/PUBLISHING_SOP.md:180-191`).
- Commit, push, tag push, visibility, and Release publication each remain
  separately gated by explicit owner authorization. Commit-only versus push is
  explicit; rollback forbids silent asset replacement and unauthorized history
  rewrite (`docs/PUBLISHING_SOP.md:3-6,135-163,193-242`).
- Phase 0 covers author email, machine paths, reports, complete reachable
  history, Actions logs/artifacts, credentials/private content, AI disclosure,
  GPL, and the need for separately approved destructive cleanup
  (`docs/PUBLISHING_SOP.md:11-33`). Phase 9 warns that making the repository
  private again cannot revoke already cloned data.

## 6. Verification evidence and remaining manual acceptance

`verify-007.md` supplies a fresh clean warnings-as-errors Windows x64 Release
build, all automated and direct watcher tests, exact install/archive evidence,
no-PDB and byte-identity checks, version/signature/hash evidence, shipped UI
matching, Markdown link/anchor checks, the mode-specific frame-decision tests,
and workflow analysis for this exact fingerprint. This guardian did not repeat
the build because no source or artifact drift exists.

The following remain owner/manual or post-authorization and are not certified
by this audit:

- install the exact final local ZIP in the supported ProgramData path, confirm
  OBS 32.1.2 loads that path/version, add the filter, and complete the
  two-minute Blocklist masking/panic smoke test;
- explicitly exercise Blocklist unverified source-plus-chip and Allowlist
  unverified full-source-plate-plus-chip behavior and recovery in OBS;
- verify a real Windows toast before content is readable, and exercise real
  Allowlist/Blocklist picker, panic, taskbar, wallpaper/sliver exclusions,
  mixed-DPI and multi-monitor placement;
- complete the 30-minute SRT soak/watcher p99, two-hour memory stability,
  60 fps impact, and fresh-machine unsigned-warning checks;
- after authorized tag push, confirm Actions and the draft prerelease,
  download back the GitHub ZIP, verify checksum/exact layout, and repeat the OBS
  smoke test;
- accept the complete public-history/private-data exposure, separately
  authorize visibility and Release publication, and perform signed-out
  repository, asset, checksum, README, LICENSE, Actions, and Issues checks.

`CHANGELOG.md:10` still says nothing has been pushed although remote `master`
equals HEAD. This is a required Phase 5 correction by the single gated scribe,
not a blocker in the hub-owned candidate. The scribe must not claim a tag,
public repository, or published Release before those actions succeed.

## 7. Final verdict and write boundary

**PASS.** The exact `verify-007` fingerprint resolves `spec-review-005` F1 and
conforms to the release, product, scope, dependency, license, privacy, and
authorization rules. Phase 4 owner-manual exact-package acceptance may proceed;
after that result is preserved, the single Phase 5 scribe may update only
`TESTING.md` and `CHANGELOG.md`.

This guardian wrote only `reports/spec-review-006.md`. It did not alter any
retained report, source, README, docs, workflows, TESTING, CHANGELOG, build, or
package artifact; did not build or launch OBS; and did not stage, commit, push,
tag, publish, change repository visibility, or perform another external
mutation. This PASS does not authorize any of those actions or convert the open
manual matrix into final acceptance.
