# Release spec review 003 — locale/UI consistency repair

- Run: 2026-07-14 +08:00
- Starting HEAD: `0be2ec9bfe2956e32fbb38f74c79ae390eed269d`
- Matching verifier: `reports/verify-003.md` (`VERIFIED`)
- Scope: the complete current uncommitted release-preparation state, including
  retained `verify-001.md`, `verify-002.md`, failed `spec-review-002.md`, the
  packaged locale repair, and the source/header comment repair
- **Verdict: FAIL**

The packaged allowlist semantics now match the final owner ruling, and every
runtime, packaging, dependency, and scope rule remains satisfied. The README
picker repair itself introduced malformed duplicated text, so the public usage
instructions are still not ready for an external reader.

## 1. Freshness and verifier match

Current `HEAD` matches the verifier's starting commit. Recomputed while
excluding `reports/verify-003.md` and this report:

```text
Tracked diff against HEAD: 11 files, 109 insertions, 63 deletions
git diff --binary --no-ext-diff HEAD | git hash-object --stdin
4a06805c938483523a7b103b763e8c325b506ceb
```

| Verified untracked input | Git blob | SHA-256 | Match |
|---|---|---|---|
| `AGENTS.md` | `ee740f9e5256efc0aa6e1a17cd34ff445e4ee724` | `3f876e77bcba0dd79714c55ee335064ff2f38a3bc37184d88db95196e20f80f2` | PASS |
| `reports/RULING-2026-07-13-noncontent-window-exclusions.md` | `297afe5f46324ca0f2ea2e837f373449a50081d9` | `b63bb1cdb9f903f082a01e4d8a42b1c937f140b1d2ef801a7e51447e42f6cdc1` | PASS |
| `reports/spec-review-002.md` | `061e38b241174883fdf2feacb5f80db336acd27a` | `1260942cf5adaea8d832c8414172bd5e27fce716aecd5fd68fc14e9988e4b26a` | PASS |
| `reports/verify-001.md` | `aa85bf8d4d9864f1234d5d4cadaee4e02ca0c1b8` | `b5a3873669641afea4dfaecdf85784fd4086d8f9d1a997cea42631c1ee9aa796` | PASS |
| `reports/verify-002.md` | `dc9e43584dfe90c0cb3c6b4e267622c415d3a877` | `f6dc2dd196ee8196845e6bd9e2cc56f4c70c9b403a06965d5a368e47fc0683b1` | PASS |

`verify-003.md` itself is unchanged (Git blob
`8a01cf5b823a116485bea18f554a4ce6853969bb`, SHA-256
`9918053b1610cd9f81ea66184239dd0d4ae3957c1efcac60be8ade2bf941b483`).
There was no post-verifier source, documentation, workflow, ruling, or retained
report drift before this report was written. `git diff --check` found no
whitespace errors; `buildspec.json` and all workflow YAML parsed successfully.

## 2. Blocking finding

### F1 — The picker README repair is duplicated and has broken Markdown

**FAIL.** The rendered control is now correctly named `Add to list`
(`data/locale/en-US.ini:15`, wired by `src/filter.c:310-312`), but the public
instruction reads:

```markdown
Pick one, press *Add
**Add to list**, and it lands ...
```

(`README.md:125-127`). This leaves an unmatched `*`, duplicates “Add”, and reads
as “press Add Add to list” rather than an exact usable control instruction.
Consequently `verify-003.md:176-180` is incorrect in concluding that README
correctly describes the exact labels, even though the label text can be found
by a string search.

The same edited documentation paragraph has a smaller subject/verb error:
plural “Claude Code and Codex workflows” is followed by “builds this plugin”
(`README.md:228-231`). It does not change product behavior, but should be fixed
in the same public-copy repair rather than published.

**Required action:** the hub should change the picker sentence to, for example,
“Pick one, press **Add to list**, and it lands ...”, and change “workflows that
builds” to “workflows that build”. Any README change invalidates this gate under
`AGENTS.md:24-25`; obtain a fresh verifier and spec review before the scribe.

## 3. Prior findings and binding-rule audit

| Rule | Result | Evidence |
|---|---|---|
| Prior F1: packaged allowlist semantics | PASS | The source, installed, and archived locale are byte-identical. `ModeAllowlist` and `AllowlistHint` now say unapproved content-bearing/application/taskbar windows are masked, an empty list approves no content-bearing window, and deterministic wallpaper exclusions remain visible (`data/locale/en-US.ini:5-9`). This matches the final ruling (`reports/RULING-2026-07-13-noncontent-window-exclusions.md:9-29`) and amended `SPEC.md:135-160`. |
| Prior F2: actual picker control labels | PASS | The shipped UI strings are exactly `Refresh list` and `Add to list` (`data/locale/en-US.ini:13-16`) and are wired through `obs_module_text` (`src/filter.c:307-312`). README names both strings, but F1 above blocks because the Add instruction surrounding the exact string is malformed. |
| Non-content implementation and comments | PASS | <=16 px windows and case-folded `Progman` / `WorkerW` are excluded before matching in both modes; taskbar and applications continue to matching (`src/watcher.cpp:306-390`). Picker enumeration mirrors the exclusions (`src/watcher.cpp:807-828`). Updated allowlist comments consistently say an empty list approves no content-bearing window and exclusions still apply (`src/watcher.cpp:377-385,742-750`; `src/watcher.h:50-54`). |
| Fail-open blocklist + visible notice | PASS | Stale/missing/mode-transition snapshots are unverified and not masked from untrusted data (`src/frame-decide.c:48-64`); the target renders with chip/log (`src/filter.c:544-592`). Monitor-enumeration degradation is published and logged (`src/watcher.cpp:463-483,528-536`). This matches `AGENTS.md:7` and `SPEC.md:221-234`. |
| Allowlist failure direction | PASS | Every unverified early path falls to mask-all (`src/frame-decide.c:39-64,74-110`), rendered as a full privacy plate with chip when degraded (`src/filter.c:521-537`), matching `SPEC.md:156-160,239-249` and the fail-open ruling addendum lines 82-93. |
| Detection/rect overflow integrity | PASS | Watcher overflow is explicit, never silently dropped (`src/watcher.cpp:283-294,510-535`); allowlist uses mask-all while blocklist keeps known masks plus a chip (`src/frame-decide.c:79-90`). |
| Coordinate confidence | PASS | Only unambiguous full-monitor geometry resolves (`src/geom-resolve.c:51-115`); mapping rejects non-finite/degenerate input and draws only `SS_MAP_OK` results (`src/coord-map.c:23-84`; `src/frame-decide.c:95-113`). |
| Toast/password guards | PASS | Toast requires process AND class plus affirmative geometry and remains ahead of allowlist approval (`src/watcher.cpp:359-385`). UIA password rects are appended independently (`src/watcher.cpp:510-524`). |
| Opaque masks and texture fallback | PASS | Full-source and per-rect plate failures both use alpha-1 solid fallback (`src/filter.c:402-416,569-584`). Verifier's clean build and four test suites, including opacity properties, pass (`reports/verify-003.md:56-115`). No blur/mosaic path exists. |
| Deterministic-only / no network inference | PASS | Runtime decisions use Win32/UIA/class/process/title/geometry only (`src/watcher.cpp:283-390`); no ML or network service dependency is present. README distinguishes deterministic runtime from AI-assisted authorship (`README.md:33-37,237-256`). |
| Watcher/render thread constraints | PASS | Shared render read is trylock-only (`src/shared-state.c:63-71`); heartbeat is published with completed snapshots (`src/watcher.cpp:528-536`), and `ss_state_touch_heartbeat` has no callers. UIA callback remains minimal. |
| Dependencies / v0.2 scope | PASS | Plugin links libobs and Windows SDK system libraries only (`CMakeLists.txt:14-17,59-63`); frontend/Qt options remain OFF (`CMakeLists.txt:7-35`). macOS/Linux CI jobs are dormant (`.github/workflows/build-project.yaml:74-80,181-189`); no backlog feature was implemented. |
| GPLv2+ / origin | PASS | Source headers use GPL version 2 or later; README states GPL-2.0-or-later and clarifies original-project provenance (`README.md:254-267`). Current source/header changes are comments only; locale text is original project copy. |
| Release package / install claims | PASS | Release excludes the PDB (`cmake/windows/helpers.cmake:25-32`). `verify-003.md:120-155` proves the exact DLL + updated locale archive, version 0.2.0, unsigned state, artifact identity, and hashes. README install/checksum/path/log claims remain consistent (`README.md:60-101`). |
| AI/distribution disclosure | PASS | README clearly discloses predominant Claude authorship, human direction, self-repository distribution, and no OBS Forum submission (`README.md:237-256`) without claiming AI runtime detection. |
| Gate sequencing / write boundary | PASS | `AGENTS.md:24-26`, `CLAUDE.md:24-25`, and `ARCHITECTURE.md:270-291` require verifier -> guardian -> scribe and prohibit external publication without owner authorization. This review followed `verify-003` and writes only this report. |
| TESTING/CHANGELOG update | PENDING SCRIBE | The final ruling requires a new verifier, guardian, and scribe (`reports/RULING-2026-07-13-noncontent-window-exclusions.md:31-35`). Because this guardian is FAIL, the scribe must refuse this pair. After a successful fresh pair, the scribe must record verified evidence and open manual acceptance only in its allowed files. |

## 4. Manual acceptance still open

`verify-003.md` provides fresh clean Release build/test/package evidence, but
the binding owner-manual checks remain uncovered: real toast timing; in-OBS
allowlist/blocklist/picker/panic/degraded and wallpaper/sliver behavior;
mixed-DPI/multi-monitor placement; the 30-minute SRT soak/p99; two-hour memory
stability and 60 fps impact; and fresh-machine install/rendered-doc accuracy.
README labels the build beta and discloses remaining checks (`README.md:9-13`),
so this audit does not convert those items into verified claims.

## 5. Final verdict

**FAIL.** The previously blocking packaged locale semantics are repaired and
all implementation/release rules pass, but the public picker instruction is
still malformed by the repair itself. The hub must correct the README copy and
obtain a fresh sequential verifier/spec review before the scribe or any release
readiness claim. No stage, commit, push, or publication is authorized by this
report.
