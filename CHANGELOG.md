# Changelog

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## Unreleased

### Renamed - 2026-07-05

Plugin renamed from `obsplugin` to **StreamSentry** (owner's choice, no
behavior change):
- `buildspec.json`: `name` → `streamsentry`, `displayName` → `StreamSentry`,
  macOS `bundleId` → `com.example.streamsentry`. `CMAKE_PROJECT_NAME` (and
  therefore `PLUGIN_NAME` and the built DLL filename) follow `name`
  automatically, so the artifact is now `streamsentry.dll`.
- Filter source id `obsplugin_filter` → `streamsentry_filter`; struct/function
  prefixes renamed to match.
- Localized display string (`data/locale/en-US.ini`) `FilterName` →
  `StreamSentry`.
- Re-verified after rename: build green (`streamsentry.dll`, 14,848 bytes),
  `dumpbin /dependents` still libobs-only (no Qt), `obs_module_load` exported,
  redeployed to `D:\software\obs\obs-studio`, OBS log confirms
  `[streamsentry] plugin loaded successfully (version 0.1.0)`.
- The repository directory itself could not be renamed from `F:\obsplugin`
  within the Claude Code session that did this work — the harness protects
  its own primary working directory from removal/rename. A full mirror
  (all git history, `.deps`, working tree) was copied via `robocopy` to
  `F:\StreamSentry`; that copy is the canonical one to use going forward.
  `F:\obsplugin` should be deleted manually (outside that session) once the
  owner has confirmed `F:\StreamSentry` works.

### 0.1.0-m0 - 2026-07-05

Milestone M0: project skeleton. Verified by `reports/M0-verifier.md`
(VERDICT: VERIFIED) and `reports/M0-spec-guardian.md` (RESULT: PASS).

- Initialized the plugin skeleton from obs-plugintemplate (upstream commit
  3e7d7ac, per project owner's record; local import commit eb47d8b).
- Plugin identity set in `buildspec.json`: name/display name `obsplugin`,
  version `0.1.0`.
- Extended the template's whitelist-style `.gitignore` to track project
  governance files (CLAUDE.md, SPEC.md, SETUP.md, TESTING.md, CHANGELOG.md,
  `.claude/`, `scripts/`, `reports/`).
- Registered a minimal pass-through video filter (source id
  `obsplugin_filter`, display name "obsplugin"): one "Enable" checkbox
  (default checked); the render callback skips the filter, so video output
  is unmodified. No detection, no masking yet.
- Template CI retained unmodified (`.github/` byte-identical to the import
  commit): pushing a tag triggers a build and a draft GitHub release
  (`.github/workflows/push.yaml`).
- Local builds use an untracked, git-ignored `CMakeUserPresets.json`
  (preset `windows-x64-local`) to select the locally installed Windows SDK
  instead of the template-pinned 10.0.22621; it does not affect CI.
