# M0 Spec Guardian Report — obsplugin

- Date: 2026-07-05
- Auditor: Spec Guardian (Claude Code agent)
- Law: F:\obsplugin\CLAUDE.md, F:\obsplugin\SPEC.md (read in full; no CLAUDE.md/SPEC.md conflict found — see Notes)
- Scope: entire tree at HEAD (1c0b2f0), covering all 3 commits:
  - eb47d8b Initialize plugin skeleton from obs-plugintemplate
  - 7db2454 Track project governance files alongside template whitelist gitignore
  - 1c0b2f0 Add minimal pass-through video filter (M0)
- M0 contract (project owner): skeleton only — filter registered, one enable
  checkbox, pure pass-through render. No detection, no masking, no fail-closed
  rendering; nothing may make future fail-closed user-disableable or harder.
- Cross-reference: reports/M0-verifier.md (VERDICT: VERIFIED) cited where noted.

---

## 1. Iron rule 2 — Deterministic only (no ML/AI): PASS

Inspected: git grep -i over all tracked files (LICENSE excluded) for
machine learning / neural / tensorflow / onnx / opencv / inference; full read
of src/plugin-main.c.

Finding: zero hits. M0 contains no detection logic of any kind, let alone
ML/AI. The only render action is obs_source_skip_video_filter()
(src/plugin-main.c:77).

## 2. Iron rule 3 — Opaque masking, no blur/pixelate/mosaic: PASS (no masking exists, as required at M0)

Inspected: git grep -i "blur|pixelat|mosaic" over all tracked files; full
read of src/plugin-main.c, data/locale/en-US.ini, properties code.

Finding: the only occurrences are the prohibitions themselves (CLAUDE.md:9,
CLAUDE.md:11, SPEC.md:17, SPEC.md:35, .claude/agents/spec-guardian.md:22).
No masking, drawing, or obfuscation code exists; no property or locale string
offers any blur/pixelate option. Properties = exactly one bool "enabled"
(src/plugin-main.c:63). Locale = FilterName + Enable only
(data/locale/en-US.ini:1-2).

## 3. Iron rule 4 — No third-party dependencies beyond obs-plugintemplate: PASS

Inspected: CMakeLists.txt, buildspec.json, scripts/bootstrap.ps1,
git diff eb47d8b HEAD on build files, reports/M0-verifier.md checks 2 and 4.

Findings:
- CMakeLists.txt:16-17 — only find_package(libobs REQUIRED) and
  target_link_libraries(... OBS::libobs). ENABLE_FRONTEND_API and
  ENABLE_QT are OFF (CMakeLists.txt:7-8). File is byte-identical to the
  template import commit.
- buildspec.json is byte-identical to the template import
  (git diff eb47d8b HEAD -- buildspec.json is empty): dependencies are exactly
  the template's own set — obs-studio 31.1.1 sources, prebuilt obs-deps,
  qt6 (template-provided). Verifier Check 2 (dumpbin /dependents) confirms the
  built DLL imports only obs.dll + VCRUNTIME/api-ms-win-crt*/KERNEL32 — Qt is
  not linked.
- scripts/bootstrap.ps1 installs only dev toolchain via winget (Git, CMake,
  VS 2022 Build Tools, OBS Studio itself). It fetches no libraries into the
  build; not a plugin dependency.

## 4. Iron rule 5 — Scope locked to v0.1 / M0 (no backlog, no premature M1+): PASS

Inspected: git ls-files src/ data/ (exactly plugin-main.c,
plugin-support.c.in, plugin-support.h, locale/en-US.ini); case-insensitive
grep over src/, data/, CMakeLists.txt for
EnumWindows|FindWindow|UIAutomation|IUIAutomation|UIA_|SetWinEventHook|DWMWA|
GetWindowRect|CreateThread|_beginthread|pthread|heartbeat|CoInitialize
and tray|auto-?update|allowlist|localiz; full read of src/plugin-main.c.

Findings:
- Zero hits on all premature-M1+ patterns: no window enumeration, no UIA, no
  watcher thread, no heartbeat, no plates, no drawing.
- Zero hits on backlog features: no tray icon, no auto-update, no per-app
  policy, no blur options, no localization beyond the template's en-US file.
- Settings surface is exactly the SPEC-sanctioned enable checkbox
  (SPEC.md:31 "Enable/disable checkbox"); nothing else.
- Fail-closed future-proofing: the sole setting toggles the whole filter — a
  SPEC-listed control. No setting, flag, or code path touches fail-closed
  semantics or could later be repurposed to disable them. Render is
  unconditional pass-through with no failure-handling branches to get wrong.
- Note (not a violation): template-provided macOS build scaffolding
  (cmake/macos/, .github macOS jobs, buildspec macos hashes) remains in tree
  from the unmodified obs-plugintemplate import. This is baseline template
  infrastructure, not implemented macOS plugin functionality; src/ contains no
  platform code at all.

## 5. Iron rule 6 — GPLv2, original code: PASS (one minor QUESTION, see below)

Inspected: LICENSE (GNU GPL Version 2, June 1991 text, verbatim);
src/plugin-main.c:1-17 GPL header with 2026 copyright; diff of plugin-main.c
against the template original (template's 34-line module skeleton retained,
filter code added fresh in 1c0b2f0).

Findings:
- LICENSE is GPLv2.
- plugin-main.c carries a GPL header and is the template skeleton plus a
  canonical minimal OBS filter written directly against the libobs API
  (obs_source_info + obs_register_source pattern per OBS headers). Nothing in
  structure, naming, or content suggests importation from another project.
  (Origin can only be audited as absence of evidence; none found.)
- QUESTION (minor): the header at src/plugin-main.c:7-8 is the template's
  stock "either version 2 ... or (at your option) any later version" grant,
  i.e. GPLv2-or-later, while CLAUDE.md rule 6 says "License is GPLv2." The
  LICENSE file governing the project is v2. This is the unmodified template
  default and v2-or-later necessarily includes v2, but if the owner intends
  strict v2-only, the header text needs the "or later" clause removed. Not
  counted as a violation; human to confirm intent.

## 6. Workflow rule — OBS 30+ APIs only: PASS

Inspected: buildspec.json:4 (obs-studio 31.1.1); every OBS symbol used in
src/plugin-main.c verified against the in-repo 31.1.1 headers (.deps/include):
- obs_source_skip_video_filter — obs.h:1430
- OBS_SOURCE_TYPE_FILTER — obs-source.h:35
- obs_source_info callbacks used (.get_defaults/.get_properties/.update/
  .video_render) — obs-source.h:283, 291, 299, 351
- obs_properties_create / obs_properties_add_bool — obs-properties.h:130, 173
- OBS_MODULE_USE_DEFAULT_LOCALE — obs-module.h:116

Finding: all APIs are current OBS 31 API; no deprecated or pre-30 constructs.
Verifier Check 3 confirms the standard OBS module ABI exports.

## 7. Local dev divergence — tracked CI/preset files match template: PASS

Inspected: git diff eb47d8b HEAD -- CMakePresets.json .github/ → empty
(byte-identical to the template import commit). Same for CMakeLists.txt,
buildspec.json, cmake/, src/plugin-support.h, src/plugin-support.c.in.
git diff --name-status eb47d8b HEAD shows the only changes since import are
governance files, .gitignore whitelist additions, src/plugin-main.c, and
data/locale/en-US.ini.

CMakeUserPresets.json exists locally (548 bytes) and is ignored
(.gitignore:2 "/*" whitelist pattern; git status --ignored lists it under
"!!"). It is not tracked and cannot affect CI.

Caveat: equivalence to the upstream obs-plugintemplate is attested by the
import commit itself; no upstream remote exists in-repo to re-verify bytes
against. What is provable: tracked CI/preset files are unchanged since import,
so CI behavior equals imported-template behavior.

## Charter minimums not separately requested

- Fail-closed integrity: NOT-APPLICABLE at M0 (no failure paths exist to
  audit; owner's contract excludes fail-closed rendering at M0). Confirmed
  nothing introduced makes it user-disableable (see section 4).
- Threading contract: NOT-APPLICABLE (no threads, no locks; render callback
  is a single skip call).
- CLAUDE.md vs SPEC.md conflict check: no conflict. Terminology note:
  CLAUDE.md:11 lists "notifications" among backlog items while SPEC.md:35
  does not; since toast *masking* is the v0.1 headline (SPEC.md:8), that entry
  can only mean plugin-emitted notifications. Zero code impact at M0.

## QUESTIONS for the human (not counted as violations)

1. TESTING.md is absent. CLAUDE.md (Workflow): "Every feature lands with a
   manual test note appended to TESTING.md." The M0 filter landed in 1c0b2f0
   with no TESTING.md; .gitignore:23 whitelists the file in anticipation, and
   the required manual in-OBS acceptance is acknowledged in
   scripts/bootstrap.ps1:58 and reports/M0-verifier.md ("Not covered by
   automation") but recorded nowhere durable. The owner scoped this audit to
   the iron rules + v0.1 scope, so this is flagged rather than folded into the
   verdict. Recommendation: do not close M0 until TESTING.md exists with the
   M0 manual test note (filter appears in list, Enable checkbox renders,
   output is pass-through).
2. src/plugin-main.c:7-8 license header is GPLv2-or-later (template stock)
   vs CLAUDE.md's "License is GPLv2" — confirm v2-only vs v2-or-later intent
   (section 5).

Observation for the implementer (no action required at M0): filter->enabled
is stored (src/plugin-main.c:42) but never read by the render path. At M1+ its
semantics must remain "whole filter inactive", never "skip fail-closed while
active".

---

RESULT: PASS
