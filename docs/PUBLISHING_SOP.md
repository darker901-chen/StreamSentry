# StreamSentry GitHub beta publishing SOP

This is the maintainer checklist for publishing StreamSentry from its own
GitHub repository. It does not authorize a commit, push, tag, visibility
change, or Release publication; each external mutation still requires explicit
owner approval.

The intended first public build is a GitHub **prerelease**, not an OBS Forum
resource submission and not a claim of final acceptance.

## Phase 0 — decide what will become public

Before changing repository visibility, review the complete Git history, not
only the current files. Making the repository public exposes commit metadata,
tracked reports, prior Actions history/logs, and every reachable commit.

Confirm all of the following:

- The maintainer accepts the public author name and email in commit metadata,
  source headers, and `buildspec.json`.
- Machine-specific drive paths and historical verification logs are acceptable
  as public development evidence.
- No private keys, credentials, stream keys, tokens, customer data, or private
  captured content are present in the tree, history, issues, Actions logs, or
  artifacts.
- The README's AI-authorship disclosure and self-repository distribution
  statement are still accurate.
- The repository is intentionally being released under GPL-2.0-or-later.

If any history item is unacceptable, stop before making the repository public.
Removing a file in a new commit does not remove it from earlier commits; history
cleanup is a separate, destructive operation that requires explicit approval
and a coordinated force-push.

## Phase 1 — prepare one exact candidate

1. Confirm the branch and worktree:

   ```powershell
   git status --short --branch
   git rev-parse HEAD
   git ls-remote origin refs/heads/master
   ```

2. Confirm public-facing version strings agree:

   - `buildspec.json`: `0.2.0`
   - README status: `0.2.0 beta`
   - package: `streamsentry-0.2.0-windows-x64.zip`
   - plugin successful-load log: `version 0.2.0`

3. Confirm README and CHANGELOG do not claim that an action has or has not
   happened when the actual GitHub state says otherwise.
4. Do not edit source or documentation after the release fingerprint is
   verified. Any change requires a new verifier and spec audit.

## Phase 2 — run the local release verification

Use the supported Windows local preset when the CI SDK preset conflicts with
the installed SDK cache:

```powershell
cmake --preset windows-x64-local -DCMAKE_COMPILE_WARNING_AS_ERROR=ON
cmake --build build_x64 --config Release --parallel -- /consoleLoggerParameters:Summary /noLogo
ctest --test-dir build_x64 -C Release --output-on-failure

build_x64\Release\coord-map-tests.exe
build_x64\Release\plate-gen-tests.exe
build_x64\Release\toast-gate-tests.exe
build_x64\Release\frame-decide-tests.exe
build_x64\Release\watcher-selftest.exe

cmake --install build_x64 --prefix "$PWD\release\Release" --config Release
Compress-Archive -Path (Get-ChildItem release\Release).FullName `
  -DestinationPath release\streamsentry-0.2.0-windows-x64.zip `
  -CompressionLevel Optimal -Force
Get-FileHash release\streamsentry-0.2.0-windows-x64.zip -Algorithm SHA256
```

The final archive must contain one `streamsentry` folder with only the release
DLL and locale data; it must not contain a PDB.

## Phase 3 — run the repository gate in order

The gate is sequential and evidence-bound:

1. The hub freezes the candidate and is the only role that edits project
   source or public documentation.
2. The verifier builds, tests, installs, packages, fingerprints the candidate,
   and writes `reports/verify-NNN.md` with `VERIFIED` or `FAILED`.
3. Only after `VERIFIED`, the spec guardian audits the same fingerprint and
   writes `reports/spec-review-NNN.md` with `PASS` or `FAIL`.
4. After `VERIFIED` + `PASS`, keep the candidate frozen and run the exact-package
   manual acceptance in Phase 4.
5. The scribe then records the machine and manual evidence once, in Phase 5,
   and updates only `TESTING.md` and `CHANGELOG.md`.
6. Any other source or public-document change after the fingerprint invalidates
   the gate. Freeze or rerun as required before tagging.

Do not run these roles in parallel.

## Phase 4 — install the exact candidate locally

Do not treat a build-tree DLL or an older development deployment as the release
artifact.

1. Install `release\streamsentry-0.2.0-windows-x64.zip` using
   `docs/INSTALLATION.md`.
2. Confirm OBS loads `version 0.2.0` from the expected path.
3. Run the two-minute masking smoke test.
4. Preserve the OBS version, Windows version, installed paths, package SHA-256,
   and result as input for the single scribe update in Phase 5.

If the exact package cannot be installed and exercised, the repository may be
shared for source review, but the binary prerelease should remain a draft.

## Phase 5 — record the final evidence

Only after `VERIFIED` + `PASS` and the Phase 4 result are available, the scribe
updates only `TESTING.md` and `CHANGELOG.md`:

- identify the matching verifier and spec-review reports;
- record the exact-package install/smoke-test result, or state clearly that it
  remains open;
- preserve every other outstanding manual acceptance item;
- correct stale publication-state wording, including claims that `master` has
  not been pushed when the remote already contains it;
- do not claim that a tag, public repository, or published Release exists until
  that action has actually succeeded.

Review the scribe-only diff before continuing. Any edit outside those two files,
or any source/public-document repair discovered at this point, requires the
candidate to return to the hub and rerun the gate.

## Phase 6 — commit and push the candidate

Only after explicit owner authorization:

```powershell
git diff --check
git status --short
git add <explicit reviewed paths>
git diff --cached --check
git diff --cached --stat
git commit -m "Prepare StreamSentry 0.2.0 beta publication"
git push origin master
```

Confirm the local and remote commit IDs match. A request to commit without a
request to push authorizes only the local commit.

## Phase 7 — create the beta tag and draft prerelease

The current GitHub Actions workflow recognizes tags such as
`0.2.0-beta1`, builds the Windows Release package, calculates checksums, and
creates a **draft prerelease**.

After explicit tag/push authorization:

```powershell
git tag -a 0.2.0-beta1 -m "StreamSentry 0.2.0 beta 1"
git show 0.2.0-beta1 --no-patch
git push origin 0.2.0-beta1
```

In GitHub Actions:

1. Open the tag-triggered **Push** run.
2. Confirm the Windows build and draft-release jobs succeed. The format job is
   intentionally skipped for a tag run; confirm the format job passed on the
   exact commit's preceding `master` push run instead.
3. Confirm the release job created a draft prerelease.
4. Confirm the draft contains
   `streamsentry-0.2.0-windows-x64.zip` and checksums in its notes.
5. Do not publish the draft yet.

If Actions fails, fix the candidate and rerun the complete gate. Do not upload a
different locally built binary under the same tag.

## Phase 8 — download-back acceptance

Download the ZIP from the GitHub draft release rather than reusing the local
file.

1. Compare its SHA-256 value with the draft release notes.
2. Inspect the archive for the exact plugin layout and absence of PDB files.
3. Install that downloaded ZIP on OBS 32.1.2.
4. Confirm the successful-load log and repeat the two-minute smoke test.

This catches upload, renaming, packaging, and stale-artifact mistakes that a
local-only test cannot catch.

## Phase 9 — make the repository public

Only after explicit visibility-change authorization:

1. Open repository **Settings**.
2. In **Danger Zone**, choose **Change repository visibility**.
3. Select **Public** and complete GitHub's confirmation.
4. In a signed-out or private browser window, confirm the repository home page,
   README, LICENSE, Issues link, Actions history, and source tree are visible.
5. Re-check that no draft release or private information became unexpectedly
   visible.

Visibility changes are not a reversible secrecy control: someone may clone or
archive the repository immediately after it becomes public.

## Phase 10 — publish the prerelease

Only after explicit Release-publication authorization:

1. Open the verified draft Release.
2. Confirm the tag is `0.2.0-beta1` and **This is a pre-release** is selected.
3. Use a title such as `StreamSentry 0.2.0 beta 1`.
4. Keep the machine-generated SHA-256 checksum in the notes.
5. Summarize supported Windows/OBS versions, unsigned status, Display Capture
   limitation, known manual acceptance gaps, and the installation SOP link.
6. Publish the prerelease.

## Phase 11 — anonymous post-publication check

In a signed-out or private browser window:

- The repository URL opens without a 404.
- The README's Releases link opens the public prerelease.
- The exact Windows ZIP downloads without authentication.
- The checksum is visible next to clear install instructions.
- GitHub Issues is available for compatibility reports.
- The source-code archives are visually distinguishable from the plugin asset.

Record the public URL, tag, commit ID, asset SHA-256, and check time. Do not
claim final acceptance while the README/TESTING manual matrix remains open.

## Stop and rollback rules

- Before public visibility: fix the candidate or keep the draft unpublished.
- After visibility becomes public: making the repository private again does not
  make already cloned data secret.
- If the binary is wrong: unpublish the Release immediately, document the
  problem, fix it under a new candidate, and avoid silently replacing a
  published asset under the same tag.
- Never rewrite public history or force-push without explicit owner approval.
