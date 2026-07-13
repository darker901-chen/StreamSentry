# Release spec review 004 — README repair

- Run: 2026-07-14 +08:00
- Starting HEAD: `0be2ec9bfe2956e32fbb38f74c79ae390eed269d`
- Matching verifier: `reports/verify-004.md` (`VERIFIED`)
- Scope: the complete current uncommitted release-preparation state, including
  all retained failed audits and verifier reports through 003, the final README
  repair, packaged locale repair, and source/header comment repair
- **Verdict: PASS**

The current diff satisfies the project law. Both earlier audit rounds remain
valid historical failures, but all of their findings are resolved in this new,
freshly verified fingerprint. No blocking finding or owner question remains.

## 1. Freshness and verifier match

Current `HEAD` matches the verifier's starting commit. Recomputed while
excluding `reports/verify-004.md` and this report:

```text
Tracked diff against HEAD: 11 files, 111 insertions, 65 deletions
git diff --binary --no-ext-diff HEAD | git hash-object --stdin
7b514f08577e0873957b48aae0a132f2fc7bd69a
```

| Verified untracked input | Git blob | SHA-256 | Result |
|---|---|---|---|
| `AGENTS.md` | `ee740f9e5256efc0aa6e1a17cd34ff445e4ee724` | `3f876e77bcba0dd79714c55ee335064ff2f38a3bc37184d88db95196e20f80f2` | PASS |
| `reports/RULING-2026-07-13-noncontent-window-exclusions.md` | `297afe5f46324ca0f2ea2e837f373449a50081d9` | `b63bb1cdb9f903f082a01e4d8a42b1c937f140b1d2ef801a7e51447e42f6cdc1` | PASS |
| `reports/spec-review-002.md` | `061e38b241174883fdf2feacb5f80db336acd27a` | `1260942cf5adaea8d832c8414172bd5e27fce716aecd5fd68fc14e9988e4b26a` | PASS |
| `reports/spec-review-003.md` | `72ccf39bc6a4e5292dd73e2c9403cc9106104361` | `98d4f48db10d25d7e7dec80ac14ceeac98c5b66ed47fa36e87493c2d4b608eb2` | PASS |
| `reports/verify-001.md` | `aa85bf8d4d9864f1234d5d4cadaee4e02ca0c1b8` | `b5a3873669641afea4dfaecdf85784fd4086d8f9d1a997cea42631c1ee9aa796` | PASS |
| `reports/verify-002.md` | `dc9e43584dfe90c0cb3c6b4e267622c415d3a877` | `f6dc2dd196ee8196845e6bd9e2cc56f4c70c9b403a06965d5a368e47fc0683b1` | PASS |
| `reports/verify-003.md` | `8a01cf5b823a116485bea18f554a4ce6853969bb` | `9918053b1610cd9f81ea66184239dd0d4ae3957c1efcac60be8ade2bf941b483` | PASS |

The matching `verify-004.md` is unchanged (Git blob
`8ab655526aaed0830099a9a1116fa245f82a0537`, SHA-256
`7de77ccd765b8364d02f923aba51f74016e848516f135f34259bebe7b9758393`).
There was no post-verifier source, documentation, workflow, ruling, or retained
report drift before this report was written. `git diff --check` passed;
`buildspec.json` and all workflow YAML parsed successfully.

## 2. Prior blocker resolution

| Prior finding | Result | Evidence |
|---|---|---|
| `spec-review-002` F1 — installed allowlist wording | PASS | Source and archived `en-US.ini` are byte-identical. `ModeAllowlist` and `AllowlistHint` say unapproved content-bearing/application/taskbar windows are masked, an empty list approves no content-bearing window, deterministic non-content exclusions remain visible, and toast/password guards remain active (`data/locale/en-US.ini:5-9`). This matches `SPEC.md:135-160` and the final ruling lines 9-29. |
| `spec-review-002` F2 — exact picker labels | PASS | The shipped UI defines `PickerRefresh="Refresh list"` and `PickerAdd="Add to list"` (`data/locale/en-US.ini:13-16`), wired to the OBS buttons at `src/filter.c:307-312`. README uses those exact labels (`README.md:124-128`). |
| `spec-review-003` F1 — malformed picker Markdown | PASS | README now reads, across normal line wrapping, “Pick one, press **Add to list**, and it lands ...” (`README.md:124-128`). CommonMark rendering produces one `<strong>Add to list</strong>` element followed by the intended sentence; the stale `press *Add` and duplicated-Add patterns occur zero times. README has 102 balanced `**` delimiter tokens. |
| `spec-review-003` grammar observation | PASS | The documentation guide now reads “Claude Code and Codex workflows that build this plugin” (`README.md:228-231`); `workflows that builds` occurs zero times and CommonMark renders it as one normal paragraph. |

## 3. Binding-rule audit

| Rule | Result | Evidence |
|---|---|---|
| Fail-open blocklist + notice integrity | PASS | Missing/stale/mode-transition snapshots are unverified and do not draw untrusted masks (`src/frame-decide.c:48-64`). The target renders with the status chip and transition log (`src/filter.c:544-592`). Monitor enumeration degradation is published and logged rather than silent (`src/watcher.cpp:463-483,528-536`). This matches `AGENTS.md:7`, `SPEC.md:221-234`, and the fail-open ruling lines 27-34. |
| Allowlist failure exception | PASS | Every unverified early path falls to mask-all (`src/frame-decide.c:39-64,74-110`); render draws the full privacy plate instead of the target and stacks the chip (`src/filter.c:521-537`). This matches `SPEC.md:156-160,239-249` and the ruling addendum lines 82-93. |
| Rect-budget integrity | PASS | Watcher sets overflow instead of silently dropping detections (`src/watcher.cpp:283-294,510-535`). Allowlist uses mask-all; blocklist keeps known masks plus a degradation chip (`src/frame-decide.c:79-90`). |
| Coordinate confidence | PASS | Geometry resolves only an unambiguous supported full-monitor capture (`src/geom-resolve.c:51-115`). Mapping rejects non-finite/degenerate input and draws only `SS_MAP_OK` results (`src/coord-map.c:23-84`; `src/frame-decide.c:95-113`). |
| Non-content exclusions | PASS | <=16 px windows and case-folded `Progman` / `WorkerW` are excluded before matching in both modes; taskbar/application windows continue to matching (`src/watcher.cpp:306-390`). Picker enumeration mirrors the same gates (`src/watcher.cpp:807-828`). Updated allowlist comments consistently state the ruled semantics (`src/watcher.cpp:377-385,742-750`; `src/watcher.h:50-54`; `ARCHITECTURE.md:99-103`). |
| Toast/password guards | PASS | Toast requires process AND class plus affirmative geometry and is handled before allowlist approval (`src/watcher.cpp:359-385`). UIA password rectangles are appended independently (`src/watcher.cpp:510-524`). |
| Opaque masks + allocation fallback | PASS | Full-source and per-rect plate texture failures both use alpha-1 solid fallback (`src/filter.c:402-416,569-584`). Plate opacity remains unit-tested; all four suites pass in `verify-004.md`. No blur/mosaic path exists. |
| Deterministic-only runtime | PASS | Runtime decisions use Win32/UIA visibility, process/title/class signatures, and deterministic geometry/list matching (`src/watcher.cpp:283-390`). There is no ML or network inference path. README distinguishes deterministic runtime from disclosed AI-assisted authorship (`README.md:33-37,238-256`). |
| Watcher/render thread constraints | PASS | Render reads shared state with trylock only (`src/shared-state.c:63-71`). Heartbeat is published with a completed snapshot (`src/watcher.cpp:528-536`), `ss_state_touch_heartbeat` has no caller, and UIA callback work remains a property read plus rect copy. |
| Dependency lock | PASS | Plugin links libobs plus Windows SDK system libraries only (`CMakeLists.txt:14-17,59-63`); frontend API and Qt remain OFF (`CMakeLists.txt:7-35`). The locale and documentation changes add no dependency. |
| v0.2 scope lock | PASS | macOS/Linux template CI jobs are dormant (`.github/workflows/build-project.yaml:74-80,181-189`). No ML, blur, per-app policy, Focus Assist integration, tray, auto-update, or other backlog implementation appears. |
| GPLv2+ and code origin | PASS | Source headers use GPL version 2 or later. README states GPL-2.0-or-later and defines original-project provenance (`README.md:254-267`). Audited source/header changes are comments only; no imported implementation or asset appears. |
| Packaging and release claims | PASS | Release install omits the PDB (`cmake/windows/helpers.cmake:25-32`). `verify-004.md` proves clean warnings-as-errors build, all tests, exact two-file package, byte-identical DLL/locale, version 0.2.0, unsigned state, and hashes. README package name, ProgramData layout, checksum-in-release-notes, log path/string, platform requirement, and warning match (`README.md:58-101`). |
| AI/distribution disclosure | PASS | README clearly separates deterministic runtime behavior from predominant Claude authorship and human direction, and says self-repository distribution with no OBS Forum submission (`README.md:33-37,238-256`). |
| Gate workflow / no external mutation | PASS | `AGENTS.md:24-26`, `CLAUDE.md:24-25`, and `ARCHITECTURE.md:270-291` record verifier -> guardian -> scribe and prohibit push/publish without owner authorization. This review followed `verify-004`, edits only this report, and performs no external mutation. |
| Required TESTING/CHANGELOG note | PASS — NEXT SCRIBE | The final ruling requires a fresh verifier, guardian, and scribe (`reports/RULING-2026-07-13-noncontent-window-exclusions.md:31-35`). The first two gates are now VERIFIED + PASS. Per `AGENTS.md:24`, the sequential scribe may now update only `TESTING.md` and `CHANGELOG.md` with verified evidence and outstanding manual checks. |

## 4. Verification and remaining manual acceptance

`verify-004.md` supplies fresh clean x64 Release build, all automated unit and
watcher tests, clean install/archive, no-PDB, artifact identity, packaged locale,
README Markdown, grammar, and exact UI-label evidence for this fingerprint.

The following remain owner-manual and are not certified by this audit: real
toast signature/geometry/timing; real in-OBS allowlist/blocklist/picker/panic/
degraded and wallpaper/sliver behavior; mixed-DPI/multi-monitor placement;
30-minute SRT soak and watcher p99; two-hour memory stability and 60 fps impact;
fresh-machine install/uninstall/security-warning UX; and rendered README/UI
accuracy. README labels the build beta and openly identifies remaining checks
(`README.md:9-13`), so no unsupported final-acceptance claim is made.

## 5. Final verdict

**PASS.** The exact `verify-004` fingerprint conforms to the binding rules and
resolves every retained audit finding. The sequential scribe may proceed within
its write boundary. This report does not authorize staging, committing, pushing,
publishing, or treating the still-open owner-manual acceptance matrix as passed.
