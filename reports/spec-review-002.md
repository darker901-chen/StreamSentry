# Release spec review 002 — v0.2.0 beta preparation

- Run: 2026-07-14 +08:00
- Starting HEAD: `0be2ec9bfe2956e32fbb38f74c79ae390eed269d`
- Matching verifier: `reports/verify-002.md` (`VERIFIED`)
- Scope: the exact current release-preparation state, including the post-M8
  field fixes in `b14b5e2` / `2023587`, the retained failed verifier report,
  and the uncommitted governance, documentation, workflow, metadata, packaging,
  and source-comment changes
- **Verdict: FAIL**

The implementation and release artifact evidence satisfy the binding masking,
failure-direction, dependency, scope, and packaging rules. Two public/user-facing
instruction sets still describe the pre-ruling UI, however. Those conflicts must
be corrected by the hub and passed through a new verifier/spec-guardian pair.

## 1. Freshness and verifier match

The verifier's starting HEAD still matches current `HEAD`. Recomputed while
excluding `reports/verify-002.md` and this report:

```text
Tracked diff against HEAD: 9 files, 98 insertions, 53 deletions
git diff --binary --no-ext-diff HEAD | git hash-object --stdin
1c36b1d81fba0d5cf9428da780ff37b5c11e746f
```

Untracked inputs that were part of verification:

| File | Git blob | SHA-256 | Match |
|---|---|---|---|
| `AGENTS.md` | `ee740f9e5256efc0aa6e1a17cd34ff445e4ee724` | `3f876e77bcba0dd79714c55ee335064ff2f38a3bc37184d88db95196e20f80f2` | PASS |
| `reports/RULING-2026-07-13-noncontent-window-exclusions.md` | `297afe5f46324ca0f2ea2e837f373449a50081d9` | `b63bb1cdb9f903f082a01e4d8a42b1c937f140b1d2ef801a7e51447e42f6cdc1` | PASS |
| `reports/verify-001.md` | `aa85bf8d4d9864f1234d5d4cadaee4e02ca0c1b8` | `b5a3873669641afea4dfaecdf85784fd4086d8f9d1a997cea42631c1ee9aa796` | PASS |

The successful verifier report itself is unchanged (`git blob
dc9e43584dfe90c0cb3c6b4e267622c415d3a877`, SHA-256
`f6dc2dd196ee8196845e6bd9e2cc56f4c70c9b403a06965d5a368e47fc0683b1`).
The retained `verify-001.md` remains a failed historical input; `verify-002.md`
supersedes it only for the fingerprint above. No post-verifier source,
documentation, workflow, ruling, or prior-report drift was present before this
report was written.

Independent static hygiene checks also passed: `git diff --check` found no
errors, and `buildspec.json` plus every `.github/workflows/*.yaml` file parsed
successfully.

## 2. Blocking findings

### F1 — User-facing allowlist text conflicts with the final non-content ruling

**FAIL.** The final ruling defines allowlist as masking unapproved
**content-bearing** windows, explicitly leaving `Progman` / `WorkerW` wallpaper
hosts and <=16 px slivers unmasked. It also says an empty allowlist approves no
content-bearing window while wallpaper remains visible
(`reports/RULING-2026-07-13-noncontent-window-exclusions.md:9-29`; mirrored by
`SPEC.md:135-150`).

The actual OBS properties still ship these pre-ruling strings:

- `data/locale/en-US.ini:5`: `Allowlist - mask everything except listed windows`
- `data/locale/en-US.ini:9`: `Only windows matching a line are shown; every
  other window is masked ... An empty allowlist masks everything.`

Those statements are materially broader than the implemented and owner-ruled
behavior in `src/watcher.cpp:312-334`. The public README now uses the correct
content-bearing wording and discloses the visible wallpaper exception
(`README.md:113-116`), so the installed UI and public documentation disagree.
This is an overstatement of privacy behavior, not a cosmetic difference.

**Required action:** the hub must revise the mode label/hint to say that
unapproved content-bearing/application and taskbar windows are masked, that an
empty list approves no content-bearing window, and that the deterministic
non-content exclusions remain visible. Because the locale is packaged into the
artifact, run a fresh verifier and spec review afterward.

### F2 — README instructs users to press picker controls that no longer exist

**FAIL.** The M8.1 field fix deliberately shortened the rendered controls to
`PickerRefresh="Refresh list"` and `PickerAdd="Add to list"`
(`data/locale/en-US.ini:13-16`; wired in `src/filter.c:307-312`). README still
instructs users to press *Add selected window to the active list* and says
*Refresh window list* re-scans (`README.md:124-128`). Those were the pre-M8.1
labels. A first-time external user following the documented literal control
names will not find them.

**Required action:** update README usage to the exact current labels, retaining
the explanation that Add routes to the active list and Refresh re-enumerates.
Run a fresh verifier and spec review because any post-gate documentation change
invalidates this fingerprint (`AGENTS.md:24-25`).

## 3. Binding-rule audit

| Rule | Result | Evidence |
|---|---|---|
| Fail-open blocklist behavior and visible notice | PASS | Binding direction is source unmodified + chip/log (`AGENTS.md:7`, `SPEC.md:221-234`, fail-open ruling lines 27-34). `ss_decide_frame` drops stale/untrusted snapshots and flags the reason (`src/frame-decide.c:48-64`); render composes the target then draws chip and transition-log (`src/filter.c:544-592`). Monitor-enumeration degradation is published and transition-logged rather than silent (`src/watcher.cpp:463-483,528-536`). |
| Allowlist failure exception | PASS | Every early unverified reason sets `mask_all` (`src/frame-decide.c:39-64,74-110`); render draws the full privacy plate instead of the target and stacks the chip (`src/filter.c:521-537`). This matches `SPEC.md:156-160,239-249` and ruling addendum lines 82-93. |
| Rect-budget behavior | PASS | Watcher marks overflow instead of silently dropping (`src/watcher.cpp:283-294,510-535`). Frame decision uses mask-all in allowlist and retained confident rects + chip in blocklist (`src/frame-decide.c:79-90`). |
| Confident coordinate mapping only | PASS | Geometry resolution accepts only an unambiguous full-monitor match and rejects unsupported/ambiguous capture geometry (`src/geom-resolve.c:51-115`). Mapping validates finite/non-degenerate inputs, intersects, pads, clamps, and distinguishes INVALID from NOT_VISIBLE (`src/coord-map.c:23-84`). Frame decision draws only `SS_MAP_OK` results and flags INVALID (`src/frame-decide.c:95-113`). |
| Non-content exclusions | PASS | The exact <=16 px geometry and case-folded `progman` / `workerw` class gates run before toast/list matching in both modes (`src/watcher.cpp:306-334`); the taskbar has no exemption and real windows continue to matching (`src/watcher.cpp:336-390`). Picker enumeration mirrors both gates (`src/watcher.cpp:807-828`). This matches the final ruling lines 9-29 and amended `SPEC.md:135-150`. |
| Toast/password guards remain active | PASS | Toast signature requires process AND class plus affirmative geometry (`src/watcher.cpp:359-374`) before allowlist matching; UIA password rect is appended independently after enumeration (`src/watcher.cpp:510-524`). Neither new exclusion changes UIA rect handling; owner-ruled <=16 px excludes only top-level window enumeration. |
| Opaque mask styles and allocation fallback | PASS | Full-source mask-all uses privacy-plate texture with alpha-1 solid fallback (`src/filter.c:402-416`); confidently mapped rect texture failure also draws alpha-1 solid (`src/filter.c:569-584`). Plate opacity is a tested property documented at `ARCHITECTURE.md:18-22,228-234`; verifier reports all four suites passed (`reports/verify-002.md`, section 3). No blur/mosaic path exists. |
| Deterministic-only runtime | PASS | Detection inputs are Win32/UIA state, process/title/class, DWM visibility, geometry, and fixed list matching (`src/watcher.cpp:283-390`). Source includes no ML/network client dependency. README carefully separates deterministic runtime from disclosed AI development authorship (`README.md:33-37,237-256`). |
| One watcher/minimal callback/non-blocking render | PASS | UIA callback reads password property and copies one rect under the password mutex; watcher folds it into a snapshot (`src/watcher.cpp:394-443,510-536`). Render reads shared state via `pthread_mutex_trylock`, never a blocking lock (`src/shared-state.c:63-71`; `ARCHITECTURE.md:32-72`). `ss_state_touch_heartbeat` has no caller; heartbeat is published only with a completed snapshot (`src/watcher.cpp:528-536`). |
| Dependencies and v0.2 scope | PASS | Plugin links libobs plus Windows SDK system libraries only (`CMakeLists.txt:14-17,59-63`); frontend API and Qt remain OFF (`CMakeLists.txt:7-8,19-35`). The changed workflow disables macOS/Linux jobs (`.github/workflows/build-project.yaml:74-80,181-189`) rather than adding platform features. No ML, blur, per-app policy, Focus Assist integration, tray, auto-update, or other backlog implementation is present. |
| GPLv2+ and origin | PASS | Source headers use GPL version 2 or later (for example `src/frame-decide.c:1-16`); README says GPL-2.0-or-later and clarifies “original” as not copied/adapted (`README.md:254-267`). The audited uncommitted source change is comment-only; no imported implementation or new asset appears. |
| Windows packaging and release claims | PASS | Release install excludes PDB while retaining it for Debug/RelWithDebInfo (`cmake/windows/helpers.cmake:25-32`). Verifier proves an exact two-file package, unsigned DLL, version 0.2.0, and matching hashes (`reports/verify-002.md`, sections 4-5). README package path, ProgramData install layout, log path, unsigned warning, and checksum-in-release-notes wording agree (`README.md:60-101`). Windows-only metadata points to the real repository (`buildspec.json:34-44`). |
| AI disclosure / distribution wording | PASS | README does not confuse AI-assisted development with runtime detection and explicitly discloses predominant Claude authorship (`README.md:33-37,237-256`). It says the project is self-distributed and not submitted to the OBS Forum; no changed file claims an OBS Forum listing. |
| Gate sequencing and write boundaries | PASS | `AGENTS.md:24-26`, `CLAUDE.md:24-25`, and `ARCHITECTURE.md:270-291` record verifier -> spec guardian -> scribe and external-publish prohibition. This review followed the successful `verify-002` sequentially and writes only this report. |
| Required manual-test/changelog note | PENDING SCRIBE | The new ruling explicitly requires verifier + spec audit + scribe (`reports/RULING-2026-07-13-noncontent-window-exclusions.md:31-35`). Workflow forbids the scribe before PASS (`AGENTS.md:24`), so no scribe update was expected in the frozen input. Because this review is FAIL, the scribe must refuse this pair. After fixes and a successful new pair, the scribe must update only `TESTING.md` and `CHANGELOG.md` with proven checks and outstanding manual acceptance. |

## 4. Verifier evidence and uncovered acceptance

`verify-002.md` is technically adequate and fresh: clean warnings-as-errors x64
Release build, ctest 4/4, all direct pure-test executables, watcher self-test,
clean install/archive, exact two-file package, no PDB, version/signature checks,
and corrected install/checksum claims all passed. The earlier `verify-001.md`
README blockers were fixed before that successful run.

The following remain owner-manual and are not converted into machine-verified
claims by this audit:

- real-toast signature/geometry/timing on supported Windows builds;
- real OBS allowlist/blocklist/picker/panic/degraded-chip/mask-all and the new
  wallpaper/sliver behavior;
- mixed-DPI/multi-monitor placement;
- 30-minute busy-desktop SRT soak and p99, two-hour memory stability, and 60 fps
  render impact;
- fresh-machine install/uninstall and rendered README/UI accuracy.

README labels the build **beta** and admits remaining real-device checks
(`README.md:9-13`), so it does not claim final acceptance. These open manual
items still prevent declaring the locked release acceptance matrix complete.
They are not the cause of this gate's FAIL; F1 and F2 are static conflicts that
can and must be corrected first.

## 5. Final verdict

**FAIL.** The masking implementation, owner-ruling application, release build,
artifact layout, and installation text pass. The installed allowlist help text
still overstates the final ruled behavior, and README names picker controls that
were removed by M8.1. The hub must fix F1/F2, then obtain a fresh sequential
verifier and spec-guardian report before the scribe or any release-readiness
claim. No push, publish, stage, or commit is authorized by this report.
