# Release spec review 005 — publication-readiness documentation

- Run: 2026-07-15 21:39 +08:00
- Starting HEAD: `7adcf43ed3b8037657463a5efbfdf668e91df1d2`
- Matching verifier: `reports/verify-006.md` (`VERIFIED`)
- Scope: modified `.gitignore` and `README.md`, plus new
  `docs/INSTALLATION.md` and `docs/PUBLISHING_SOP.md`; retained failed history
  in `reports/verify-005.md`
- **Verdict: FAIL**

The installation paths, package identity, UI labels, two-minute blocklist smoke
test, publication phases, GitHub tag workflow, authorization boundaries, and
both `verify-005` process repairs are sound. One public behavior claim is still
incorrect for allowlist mode: the documentation describes blocklist fail-open
rendering as if it applied to both modes. This is a release-blocking semantic
error because allowlist deliberately falls to a full-source privacy plate plus
the status chip whenever protection is unverified.

## 1. Freshness and verifier match

Current `HEAD`, local `master`, `origin/master`, and the remote `master` ref all
resolve to `7adcf43ed3b8037657463a5efbfdf668e91df1d2`.

Recomputed immediately before this report while excluding only retained
`reports/verify-005.md`, matching `reports/verify-006.md`, and this new report:

```text
Tracked diff against HEAD: 2 files, 48 insertions, 24 deletions
git diff --binary --no-ext-diff HEAD | git hash-object --stdin
4057c4fcbc2f60de8dc0268b8a0a4c82142e1e21
```

| Untracked candidate input | Git blob | SHA-256 | Match |
|---|---|---|---|
| `docs/INSTALLATION.md` | `48aee785bcf94a75643e06236c261b1700e505a6` | `d05bb664e8aa8ca56b6fdcdcc0130316556cf887dc62bd6b6827b0a6421512f2` | PASS |
| `docs/PUBLISHING_SOP.md` | `2403948b6b6c455768b4c254368bed10946bda69` | `004b08780afd55eaab90b6259add0e80a4ad95e03f970fccd389f20e82dc0253` | PASS |

Retained `reports/verify-005.md` remains byte-identical to the verifier's
recorded failed history, SHA-256
`bd9cb4354c9c0c0b0bac17ca96c3aed4ec2d087753d5dd3671465b3fc00d2028`.
The matching `verify-006.md` is unchanged (Git blob
`52d168630d9c5e4f654fc66ac013cff861cd6c06`, SHA-256
`ffb8e19746fbbad6a5248e79e37bcd95e2dece0fdf39e7ae7614dbfc67ed2840`).
`git diff --check` passes. No post-verifier candidate drift occurred before this
report was written.

## 2. Blocking finding

### F1 — Unverified-capture wording omits the allowlist mask-all exception

**FAIL.** The new installation SOP first asks the user to keep Blocklist for
the smoke test, but then makes an unqualified product statement that unsupported
Window Capture, cropped/scaled Display Capture, and ambiguous monitor layouts
keep rendering the source with a degradation chip
(`docs/INSTALLATION.md:93-104`). The README repeats the same behavior without a
mode qualification in both its overview and limitations
(`README.md:39-50,162-167`). A user can reasonably read these statements as
applying after switching the documented **Masking mode** to Allowlist.

That is not the specified or implemented allowlist behavior:

- `SPEC.md:156-160,239-249` requires stale heartbeat, unresolved geometry,
  mapping failure, and every other unverified allowlist state to produce a
  full-source **mask-all privacy plate** plus the status chip. This is the
  default-deny direction the user explicitly selected, not raw black and not
  silent pass-through.
- The final fail-open ruling records the same mode exception, including
  allowlist `detection_degraded` behavior
  (`reports/RULING-2026-07-09-fail-open.md:27-40,82-93`).
- The implementation sets `mask_all` on every unverified allowlist early-out,
  including unresolved geometry and coordinate mapping failure
  (`src/frame-decide.c:39-77,95-111`), then renders the full privacy plate and
  stacks the chip (`src/filter.c:521-537`). Blocklist alone preserves the source
  on these failure paths.

This matters operationally: a tester using allowlist on an unsupported or
ambiguous capture will see the whole source plated, while the current public
documentation promises normal rendering. That can be mistaken for a plugin
failure and contradicts the owner-approved mode design.

**Required repair:** qualify the fail-open wording everywhere it is presented
as general behavior. State explicitly that Blocklist renders the source
unmodified with the degradation chip when protection cannot be verified, while
Allowlist falls to a full-source opaque privacy plate with the chip because it
is default-deny. At minimum this must repair `README.md:39-50,162-167` and
`docs/INSTALLATION.md:101-104`. Any public-document repair changes the verified
fingerprint, so the hub must make the edit and obtain a fresh verifier and spec
review before Phase 4.

## 3. `verify-005` blocker resolution

| Prior finding | Result | Evidence |
|---|---|---|
| F1 — impossible green format job on a tag run | PASS | The repaired SOP requires the Windows build and draft-release jobs to succeed, explicitly recognizes that format is skipped on tag runs, and binds format evidence to the exact commit's preceding `master` push run (`docs/PUBLISHING_SOP.md:166-175`). This matches `.github/workflows/push.yaml:14-32`. |
| F2 — scribe ran before exact-package manual acceptance | PASS | The gate freezes VERIFIED + PASS for Phase 4, preserves the exact-package result, and performs one scribe update afterward in Phase 5 (`docs/PUBLISHING_SOP.md:81-133`). This gives the scribe the real package evidence before it edits only `TESTING.md` and `CHANGELOG.md`. |

The Phase headings form the uninterrupted sequence 0 through 11. Commit/push,
tag/draft, download-back, visibility, release publication, and anonymous checks
remain separate and ordered.

## 4. Remaining binding-rule audit

| Rule | Result | Evidence |
|---|---|---|
| Standard OBS installation | PASS | README and installation SOP distinguish the actual Release asset from GitHub source archives, close OBS, copy the single self-contained `streamsentry` folder, name the exact ProgramData DLL/locale paths, and call out accidental double nesting (`README.md:64-98`; `docs/INSTALLATION.md:7-48`). |
| Custom/portable installation | PASS | The SOP maps the DLL to `obs-plugins\64bit` and locale to `data\obs-plugins\streamsentry\locale`, uses the OBS log to identify the loaded copy, and warns against duplicate versions (`docs/INSTALLATION.md:50-73`). This agrees with the verified two-file package. |
| Load and smoke-test claims | PASS | Version `0.2.0`, filter name, **Open windows**, **Add to list**, **Refresh list**, default Blocklist mode, and panic-hotkey label match `data/locale/en-US.ini:1-16` and their wiring in `src/filter.c`. The smoke test narrowly claims loading, filter creation, picker-to-blocklist routing, opaque masking, and panic on that installation, and explicitly does not claim broader acceptance (`docs/INSTALLATION.md:75-125`). |
| Package, version, and signing | PASS | `buildspec.json` is `0.2.0`; Release installation targets `streamsentry/bin/64bit` plus `streamsentry/data`, and the Release configuration omits the optional PDB. `verify-006.md` independently proves the exact two-file ZIP, version resources, unsigned status, hashes, and byte identity. Public docs name the same ZIP and disclose the unsigned beta. |
| Opaque masking / deterministic runtime | PASS | The candidate adds no runtime code or dependency. README retains deterministic Win32/UIA behavior, no AI inference or captured-content service, opaque-only masking, and no blur/pixelation (`README.md:31-56`). |
| v0.2 scope | PASS | The docs describe only shipped v0.2 behavior and installation/publication work. They add no macOS, AI detection, blur, per-app policy, Focus Assist integration, tray icon, auto-update, or other backlog feature. The publishing SOP correctly treats dormant macOS/Linux jobs as outside the Windows beta surface. |
| GPL and source availability | PASS | README and publication Phase 0 identify GPL-2.0-or-later (`README.md:290-293`; `docs/PUBLISHING_SOP.md:17-33`). The source repository becomes public before the binary prerelease is published, providing equivalent source access with the tagged binary surface. No dependency or imported code is added. |
| Tag and GitHub Release workflow | PASS | `0.2.0-beta1` is accepted by the prerelease tag case in `.github/workflows/push.yaml:44-55` and selects Release configuration in `.github/workflows/build-project.yaml:43-48`. The Windows job packages/uploads the documented artifact; the tag release job downloads it, generates SHA-256 notes, and creates a draft prerelease (`.github/workflows/push.yaml:58-115`). The SOP requires download-back verification before visibility/publication. |
| Authorization and rollback boundaries | PASS | The SOP disclaims authority at entry, repeats explicit authorization before commit/push, tag push, visibility, and Release publication, preserves commit-only versus push semantics, and forbids silent asset replacement/history rewrite (`docs/PUBLISHING_SOP.md:3-9,135-163,193-242`). This agrees with `AGENTS.md`. |
| Public-history and privacy warning | PASS | Phase 0 names commit metadata, author email, machine paths, reports, Actions logs/artifacts, credentials/private content, AI disclosure, and destructive history cleanup before visibility (`docs/PUBLISHING_SOP.md:11-33`). Phase 9 warns that returning private cannot revoke clones (`docs/PUBLISHING_SOP.md:193-206`). |
| Stale publication wording | **REQUIRED NEXT SCRIBE** | `CHANGELOG.md:10` still says nothing has been pushed, although remote `master` equals HEAD. Phase 5 explicitly requires the single scribe to correct it without falsely claiming a tag, public repository, or published Release (`docs/PUBLISHING_SOP.md:119-133`). Because this guardian is FAIL, the scribe must not proceed on this pair. |

## 5. Verification evidence and remaining manual acceptance

`verify-006.md` supplies a fresh clean warnings-as-errors Windows x64 Release
build, all automated and direct watcher tests, clean install/package evidence,
no-PDB and byte-identity checks, version/signature/hash evidence, UI-label
matching, Markdown link/anchor checks, and workflow analysis for this exact
fingerprint. This guardian did not repeat the build because no source or
artifact drift was present and the failure is a documentation/spec mismatch.

The following remain manual and are not accepted by this audit:

- install the exact final local ZIP in the supported ProgramData path, confirm
  OBS 32.1.2 loads that path and version, add the filter, and complete the
  two-minute blocklist masking/panic smoke test;
- verify a real Windows toast before content is readable;
- exercise in-OBS blocklist, allowlist default-deny and unverified mask-all,
  picker, panic, degraded chip, taskbar, wallpaper/sliver exclusions, and
  recovery;
- verify mixed-DPI/multi-monitor placement, 30-minute SRT soak and watcher p99,
  two-hour memory stability, 60 fps impact, and fresh-machine unsigned-warning
  UX;
- after an authorized tag push, confirm Actions and the draft prerelease,
  download back the GitHub ZIP, verify checksum/exact layout, and repeat the OBS
  smoke test;
- accept the complete public history/private-data exposure, explicitly
  authorize visibility and Release publication, and perform signed-out
  repository, asset-download, checksum, README, LICENSE, Actions, and Issues
  checks.

## 6. Final verdict and write boundary

**FAIL.** The `verify-005` F1/F2 repairs are correct and the candidate is strong
on installation mechanics and publication safety, but its unqualified
unverified-capture wording contradicts the binding allowlist mask-all exception.
The hub must repair the public documentation and rerun verifier -> guardian.
The scribe and Phase 4 exact-package acceptance must not proceed on this failed
pair.

This guardian wrote only `reports/spec-review-005.md`. It did not edit source,
README, docs, workflows, TESTING, CHANGELOG, or any existing report; did not
build or launch OBS; and did not stage, commit, push, tag, publish, change
repository visibility, or perform any other external mutation.
