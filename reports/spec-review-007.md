# Release spec/security review 007 — panic-hotkey removal

- Run: 2026-07-15 23:12 +08:00
- Starting HEAD: `7adcf43ed3b8037657463a5efbfdf668e91df1d2`
- Matching verifier: `reports/verify-008.md` (`VERIFIED`)
- Authority: `reports/RULING-2026-07-15-remove-panic-hotkey.md`
- Scope: the complete current workspace candidate before this report,
  including the owner ruling and retained publication-gate history
- **Verdict: PASS**

The exact verified candidate implements the owner's removal decision without
weakening StreamSentry's default-deny protection. No panic/hotkey registration,
state, callback, label, binary string, current user workflow, publishing step,
or shipping acceptance row remains. Allowlist unverified states still produce
the full-source opaque privacy plate plus the degradation chip, while Blocklist
unverified states preserve the source and show the chip. The shipping v0.2
scope is now the four retained items only.

## 1. Freshness and verifier match

Current `HEAD`, local `master`, `origin/master`, and the remote `master` ref all
resolve to `7adcf43ed3b8037657463a5efbfdf668e91df1d2`.

Recomputed immediately before this report while excluding only matching
`reports/verify-008.md` and this new report:

```text
Tracked diff against HEAD: 10 files, 261 insertions, 117 deletions
git diff --binary --no-ext-diff HEAD | git hash-object --stdin
85e299f92c691e810014117e58252e98f3e8f260
```

Every other untracked input is part of the candidate and matches the verifier:

| Untracked candidate input | Git blob | SHA-256 | Match |
|---|---|---|---|
| `docs/INSTALLATION.md` | `2eabf8b342f80dadbfe1c7f23745a61864c272eb` | `8d9bc3fc19041650268a371cbd972771f7412fc47f5924cd071b28a52f31bd1d` | PASS |
| `docs/PUBLISHING_SOP.md` | `147c89396ebeed9fc4f894e705ff64b9e2ad0511` | `e945c592999264debd9604a59d08ec001b69ad863be3db2ee7a12b17616ef797` | PASS |
| `reports/RULING-2026-07-15-remove-panic-hotkey.md` | `a1b03ef26ba3d3098f97b5aeecb6e8ae9e7b95d4` | `2c426d6a591f2e78a051daff45acd186777af72a9686fdfcb6c1315eb7dc53c8` | PASS |
| `reports/spec-review-005.md` | `f02f9354eed9b6c748d7ecf4f22733db4c3e993b` | `ba2c713ccb0617f0b5e01af01a5392ca248ffeb8867aff2cb9da37966730e026` | PASS |
| `reports/spec-review-006.md` | `43025c8edfabd42004fc119f2af7c9716456c3cd` | `bef041b8fd0663662d26e8a752b512bc5ee9db64e4b0861c2860126d069709b8` | PASS |
| `reports/verify-005.md` | `1d61e1d8037adf76bf30018138e895831ff4741f` | `bd9cb4354c9c0c0b0bac17ca96c3aed4ec2d087753d5dd3671465b3fc00d2028` | PASS |
| `reports/verify-006.md` | `52d168630d9c5e4f654fc66ac013cff861cd6c06` | `ffb8e19746fbbad6a5248e79e37bcd95e2dece0fdf39e7ae7614dbfc67ed2840` | PASS |
| `reports/verify-007.md` | `b59dc6fdc380acbf923e9a177cae1e208bd0bd6e` | `b82d74b3e31946a6307a6bb7939aea9de5aa233cb6271ab510426e4fac937238` | PASS |

The matching `verify-008.md` is unchanged (Git blob
`1965ecc09f543d6ff0746c10afb3136f00452bee`, SHA-256
`de50221997029b3e64a05d71e571775692a363492ba96a1ca484c047c872b4f1`).
`git diff --check` passes. No substantive source, locale, governance, docs,
workflow, ruling, TESTING, CHANGELOG, or retained-report drift occurred before
this report was written.

## 2. Owner ruling and four-item shipping scope

**PASS.** The 2026-07-15 ruling is explicit and internally consistent:

- panic was experimental scaffolding, not a shipping requirement;
- the beta registers no StreamSentry hotkey, retains no panic state or toggle,
  ships no `PanicHotkey` locale entry, and advertises no panic workflow;
- historical milestone/gate evidence stays unchanged;
- Allowlist mask-all remains deterministic default-deny behavior, not a renamed
  panic control; Blocklist still renders the source with the degradation chip.

`AGENTS.md:4,11` and `CLAUDE.md:4,11` remove panic from the product description,
lock v0.2 to four shipping items, and cite the owner removal. `SPEC.md:69-75`
defines four shipping items; the actual retained items are watcher performance
hardening (2.1), toast geometry narrowing (2.2), Allowlist mode (2.3), and the
window picker (2.5). Section 2.4 is now an explicit removal record, with no
shipping registration/state/workflow or acceptance row (`SPEC.md:167-174,
257-263`). The Chromium investigation remains documentation-only and
non-gating, while 2.7 remains the owner-approved failure-semantics ruling; they
do not add shipping items.

No backlog item was introduced. The diff contains only publication
documentation/history, governance/spec/architecture synchronization, one locale
deletion, and removal of the source control/state/render branch.

## 3. Complete removal from the shipping surface

### 3.1 Source and locale

**PASS.** A case-insensitive recursive scan of every file under `src/` and
`data/locale/` found zero instances of `panic`, `hotkey`, `obs_hotkey`,
`PanicHotkey`, or `streamsentry.panic`. The only related natural-language hit
was `frame-decide.c:40` saying Allowlist falls to its default and masks
everything; that is the required default-deny behavior.

The source diff removes all of the former shipping mechanism from
`src/filter.c`:

- no panic boolean or `obs_hotkey_id` remains in `struct ss_filter`
  (`src/filter.c:65-92`);
- no callback, registration during create, or unregistration during destroy
  remains (`src/filter.c:100-147`);
- `draw_full_plate` is documented solely as Allowlist mask-all
  (`src/filter.c:376-390`);
- the full-source branch is now only `if (dec.mask_all)`, with no atomic panic
  condition (`src/filter.c:494-508`).

`data/locale/en-US.ini:1-15` contains only enable, mode, Blocklist, Allowlist,
picker, refresh, and add strings. `PanicHotkey` and the prior user label are
absent. The verified clean warnings-as-errors Release build proves no dangling
callback/state reference or warning remains.

### 3.2 Release DLL and archive

**PASS.** This guardian independently read the verified installed Release DLL
as ASCII and UTF-16 and found all of these tokens absent:

```text
panic
hotkey
obs_hotkey
PanicHotkey
streamsentry.panic
mask everything
```

The DLL is 76,288 bytes with SHA-256
`a9da896362379153953ebf05d454504956ff776a57ea512a7a57b402b315fb2d`,
matching `verify-008`. The final ZIP SHA-256 is
`5d1e7d8779899c9b21b46e0b6946f82504ef14ffa232174e47b4a8ca26710018`.
A read-only archive listing contains exactly the Release DLL (76,288 bytes) and
locale (1,377 bytes), beneath one `streamsentry` root, plus directory entries;
there is no PDB or extra payload. The verifier additionally checked imports and
the built/installed byte identity.

### 3.3 Current user, publishing, and workflow text

**PASS.** Case-insensitive scans of `README.md`, `docs/INSTALLATION.md`, and
`docs/PUBLISHING_SOP.md` return zero `panic`, `hotkey`, or removed-label hits.
The README usage surface ends with mode/list/picker and the mandatory degraded
chip (`README.md:120-151`). The installation smoke test now covers load,
Blocklist picker/add/remove, opaque masking, and log inspection only
(`docs/INSTALLATION.md:93-123`). The publishing SOP calls it a two-minute
masking smoke test and never asks the maintainer to bind or test a removed
control (`docs/PUBLISHING_SOP.md:102-112,180-190`).

A recursive scan of `.github` workflows/actions also found no panic/hotkey
term. Packaging and GitHub Release behavior are unchanged.

## 4. Allowlist and Blocklist security semantics

**PASS.** Removing a user-triggered full-source plate did not remove or weaken
the security-directed full-source path:

- `SS_ALLOWLIST_FALLBACK()` still sets `out->mask_all` for missing/stale
  snapshots, mode transition, watcher-side detection degradation, unresolved
  capture geometry, and invalid mapping (`src/frame-decide.c:39-77,95-111`).
- Allowlist rect-budget overflow still sets `mask_all`; Blocklist overflow keeps
  published confident rects and reports degradation (`src/frame-decide.c:79-90`).
- `src/filter.c:494-508` draws the full opaque privacy plate instead of the
  target whenever `dec.mask_all` is set and stacks the chip when unverified.
  Texture allocation failure still falls back to an alpha-1 solid fill.
- Blocklist retains target rendering and the degradation chip on unverified or
  filter-chain-bypass paths, while Allowlist retains the full plate
  (`src/filter.c:515-536,540-566`).

This agrees with the fail-open ruling, the new removal ruling, AGENTS/CLAUDE
iron rule 1, `SPEC.md:151-160,224-250`, the mode-qualified README opening and
limitations, and `docs/INSTALLATION.md:101-106`. `verify-008` reports every
direct frame-decision case still passing. The remaining `mask_all` path is
therefore not hidden panic state; it is solely the deterministic Allowlist
default and overflow policy.

## 5. Governance, architecture, dependency, and license consistency

| Rule | Result | Evidence |
|---|---|---|
| AGENTS / CLAUDE | PASS | Both remove panic from the product summary, state four shipping items, record the owner removal, and preserve deterministic-only, opaque-only, no-new-dependency, GPLv2+, confidence, and Allowlist-exception rules (`AGENTS.md:4-12`; `CLAUDE.md:4-12`). |
| SPEC | PASS | Part 2 states four shipping items; 2.4 is removal history; current acceptance removes both panic rows but retains Allowlist overflow/watcher-kill mask-all (`SPEC.md:69-75,151-174,257-263`). |
| ARCHITECTURE | PASS | `filter.c` ownership no longer names hotkey state; the render diagram has only `mask_all -> full-source plate`; the callback/atomic/persistence paragraph is removed. Frame-decision and mode-specific degradation descriptions remain accurate (`ARCHITECTURE.md:19-25,116-132,150-181`). |
| README / installation / publishing docs | PASS | Current user-facing and maintainer workflow text contains no removed control and remains mode-qualified, package-accurate, and honest about manual acceptance. |
| Dependencies | PASS | `CMakeLists.txt` is unchanged: shipping links libobs and Windows SDK system libraries; frontend API and Qt remain OFF (`CMakeLists.txt:7-26,62`). Removal adds no library, network service, runtime inference, or installer dependency. |
| Backlog scope | PASS | No macOS/Linux runtime support, notification implementation, blur/mosaic, per-app policy, Focus Assist integration, tray icon, auto-update, AI detection, or content-secret scanner was added. Dormant non-Windows template jobs remain unchanged. |
| GPL/origin | PASS | Source GPL-2.0-or-later headers remain intact; README retains GPL-2.0-or-later and original-project provenance (`README.md:275-289`). No third-party implementation or asset was introduced. |

## 6. Package and publication SOP honesty

**PASS.** README and installation SOP still identify the correct
`streamsentry-0.2.0-windows-x64.zip`, distinguish it from GitHub source
archives, disclose unsigned status, state the ProgramData self-contained DLL
and locale paths, map custom/portable OBS to the split traditional layout, and
cover duplicate copies, update, uninstall, versioned load logs, checksum, and
private-log redaction. Their shipped UI labels match the 15-line locale and
`src/filter.c` properties.

The publishing SOP still has the exact Phase 0-through-11 order. It requires
VERIFIED then PASS, the exact local package smoke test, one scribe update,
separately authorized commit/push and tag push, tag Actions/draft verification,
download-back acceptance, separately authorized public visibility and Release
publication, and anonymous checks. `0.2.0-beta1` still selects the Release
Windows package and draft prerelease; no removed control appears in the
workflow. Public-history/privacy, author-email/machine-path, irreversible
visibility, no-silent-asset-replacement, and no-unauthorized-force-push warnings
remain intact.

## 7. Historical boundary and required next scribe

Historical M7 sections and older gate reports correctly remain unchanged. They
describe what earlier fingerprints implemented and tested, and the new owner
ruling explicitly supersedes them for the shipping candidate. They must not be
rewritten as if panic never existed.

The newest scribe-owned publication entries in `CHANGELOG.md` and `TESTING.md`
still bind the immediately preceding `verify-007` / `spec-review-006`
fingerprint and therefore still contain panic smoke/manual steps, old artifact
sizes, and the prior ZIP hash (`CHANGELOG.md:15-66`;
`TESTING.md:1336-1438`). This is **required next-scribe work, not a guardian
failure**: project law permits the scribe to update these two files only after
VERIFIED + PASS, and that gate is completed by this report.

Before commit/tag/publication, the single sequential scribe must:

- make `verify-008` + `spec-review-007` the authoritative current pair;
- record 76,288-byte DLL, 1,377-byte locale, and the verified current ZIP hash;
- remove panic/hotkey from the newest current smoke and pending-manual matrix;
- add the remaining manual check that OBS Settings -> Hotkeys contains no
  StreamSentry hotkey;
- preserve older M7 milestone text and old reports unchanged;
- retain all other manual and publication gaps and avoid claiming tag, public
  visibility, or published prerelease before those actions occur.

No source, governance, specification, architecture, README, docs, locale,
workflow, or owner-ruling repair is needed. Any such change would return to the
hub and require a new verifier/guardian fingerprint.

## 8. Remaining manual and post-authorization checks

- Install this exact ZIP under the supported ProgramData path, confirm OBS
  32.1.2 loads version 0.2.0 from it, add the filter, and complete the current
  non-panic Blocklist picker/add/remove masking smoke test.
- Confirm no StreamSentry entry appears in OBS Settings -> Hotkeys.
- Exercise Blocklist unverified source-plus-chip and Allowlist unverified
  full-source-plate-plus-chip behavior and recovery in OBS.
- Verify real Windows toast signature/geometry/timing, real Blocklist/Allowlist
  picker behavior, taskbar and wallpaper/sliver exclusions, and mixed-DPI/
  multi-monitor placement.
- Complete the 30-minute SRT soak/watcher p99, two-hour memory stability,
  60 fps impact, and fresh-machine unsigned-warning/install/uninstall checks.
- After explicit authorization, confirm tag Actions and draft creation,
  download back the GitHub ZIP, validate checksum and exact layout, retest OBS,
  accept public-history/privacy exposure, separately authorize visibility and
  Release publication, and run the anonymous repository/asset/Issues checks.

## 9. Final verdict and write boundary

**PASS.** The exact `verify-008` candidate implements the 2026-07-15 removal
ruling, ships the four-item v0.2 scope without a panic/hotkey surface, preserves
Allowlist default-deny and Blocklist fail-open-notice semantics, and satisfies
dependency, GPL, package, documentation, and publishing-workflow constraints.
The sequential scribe may now update only `TESTING.md` and `CHANGELOG.md` as
listed in section 7; this PASS does not authorize any external mutation or
claim final acceptance.

This guardian added only `reports/spec-review-007.md`. It did not edit source,
governance, specification, architecture, README, docs, TESTING, CHANGELOG,
locale, workflows, owner ruling, verifier evidence, or any existing report; it
did not build or launch OBS, stage, commit, push, tag, publish, change
visibility, or perform another external mutation.
