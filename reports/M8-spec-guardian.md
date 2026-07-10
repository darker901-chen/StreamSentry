# M8 Spec-Guardian Audit -- window picker + docs/package refresh + v0.2 FINAL report (SPEC 2.5)

- Date: 2026-07-10
- Auditor: spec-guardian (Fable 5)
- Law: CLAUDE.md (as amended 2026-07-09) + SPEC.md (Part 1 + Part 2, 2.5/2.7
  governing) + reports/RULING-2026-07-09-fail-open.md incl. both addenda
- Change set: staged diff at HEAD 706cda1 (9 files: ARCHITECTURE.md, README.md,
  buildspec.json, data/locale/en-US.ini, reports/V02-FINAL.md (new), src/filter.c,
  src/watcher.cpp, src/watcher.h, tests/watcher-selftest.cpp; no unstaged edits)
- CLAUDE.md vs SPEC.md conflict check: NONE -- both carry the 2026-07-09
  amendments consistently (iron rules 1/3 <-> SPEC 2.7).

## 1. SPEC 2.5 -- window-picker UI, line by line

| Rule | Verdict | Evidence |
|---|---|---|
| "List open windows" affordance in filter properties | PASS | filter.c:300-307 -- combo "picker_window" (pre-filled on dialog open via picker_fill_combo), "picker_refresh" button (re-enumerate), "picker_add" button (append). SPEC 2.5: "exact widget shape is implementation-defined within stock OBS properties". |
| Stock OBS properties only, no custom Qt (iron rule 4) | PASS | Only obs_properties_add_list / obs_property_list_clear / obs_property_list_add_string / obs_properties_add_button / obs_properties_add_button2 / obs_properties_get (filter.c:209-307) plus obs_data_* / obs_source_get_settings / obs_source_update. No Qt, no custom widgets, no new headers beyond ctype.h/stdio.h (CRT). |
| Entries show process image name + window title | PASS | picker_fill_combo filter.c:214-218: label "proc - title" (title part omitted when empty); wins[] from ss_enum_open_windows carries both fields (watcher.h:64-67). See observation O2 (over-long titles). |
| Selection appends to the ACTIVE list per current mode | PASS | picker_add_clicked filter.c:246-247: key is "allowlist" iff settings "mode" equals "allowlist", else "blocklist". Matches the mode property's stored values exactly (filter.c:286-287), same comparison as filter_update (filter.c:108) and mode_modified (filter.c:270). Missing/unset key falls to "blocklist" -- same safe direction as the rest of the code and the SPEC 2.3 upgrade default (filter.c:317 sets the default anyway). Reading "mode" from live settings means the append follows the mode currently selected in the dialog. |
| Appended as process-name lines, no hand-typing | PASS | Combo item value = wins[i].proc (filter.c:218), lowercase process image name (watcher.h:65; proc_image_name lowercases, watcher.cpp:134). Append joins with a newline (filter.c:250-255); first entry into an empty list gets no leading separator. |
| The multiline textbox remains; picker only appends | PASS | Both OBS_TEXT_MULTILINE boxes unchanged (filter.c:293-298); the picker path only ever writes the joined text of the active key -- no removals, no reformatting of existing lines. |
| Duplicate selections do not duplicate entries | PASS | list_has_line (filter.c:178-207) gates the append (filter.c:249): case-insensitive exact-line compare, trims spaces/tabs per line (filter.c:189-194), splits on both LF and CR (filter.c:186) so CR/LF and CRLF texts are handled (a CRLF pair yields one extra empty-line iteration, which cannot match a non-empty proc). Selftest asserts enumeration-side dedupe (tests/watcher-selftest.cpp:171-183). |

## 2. Picker enumeration (ss_enum_open_windows)

| Rule | Verdict | Evidence |
|---|---|---|
| Same visibility/cloak gates as detection | PASS | pick_enum_proc watcher.cpp:794-801 (IsWindowVisible -> cloak_state -> GetWindowRect nonempty) mirrors enum_proc watcher.cpp:296-311. |
| CLOAK_UNKNOWN excluded from the UI list | PASS | watcher.cpp:796: skip unless cloak_state == CLOAK_NO -- excludes both CLOAK_YES and CLOAK_UNKNOWN ("only confidently displayed windows"). Stricter than detection's mode-aware handling (watcher.cpp:299-304, ruling addendum 2 item 6), in the mask-more direction: an unpickable window cannot be approved via the picker, so in allowlist mode it STAYS masked. Cannot weaken masking. |
| Cannot weaken any masking path | PASS | ss_enum_open_windows (watcher.cpp:840-850) writes only the caller-supplied out buffer; it never touches ss_state_*, rects, flags, or the heartbeat. Its only callers are picker_fill_combo (filter.c:213) and the selftest (tests/watcher-selftest.cpp:174) -- grep-verified; detection (enum_proc/tick) is untouched by this diff. |
| UI thread does NOT touch the watcher-thread-only PID cache | PASS | pick_enum_proc calls proc_image_name(pid) directly (watcher.cpp:805). cached_proc_image_name / g_pid_cache / pid_cache_begin_tick / pid_cache_end_tick are referenced only from the watcher tick path (watcher.cpp:328, 465-467); ownership comment watcher.cpp:144-145. proc_image_name itself is stateless (OpenProcess/QueryFullProcessImageNameW/CloseHandle, watcher.cpp:120-135). |
| No locks shared with the render path | PASS | ss_enum_open_windows takes no locks at all; cloak_state is a stateless DwmGetWindowAttribute call (watcher.cpp:218-230). The render callback (filter_video_render) is byte-identical in this diff. |
| Buffer/ABI safety (spot check) | PASS | utf16_to_utf8 NUL-guarantees (watcher.cpp:776-786); memcpy of proc8 (max 63 bytes) into the zeroed 64-byte field (watcher.cpp:831-833); EnumWindows stops at max_count (watcher.cpp:790-791); struct + function declared inside watcher.h's extern "C" block (watcher.h:31-80) so the C caller links correctly. |

## 3. Settings safety

| Rule | Verdict | Evidence |
|---|---|---|
| Writes via obs_source_update on the filter's own settings object | PASS | filter.c:243 obs_source_get_settings -> obs_data_set_string on "blocklist"/"allowlist" -> obs_source_update(f->source, settings) -> obs_data_release (filter.c:256-261). cur is consumed (strlen + snprintf) before the set -- no use-after-invalidate. NULL data (obs_get_source_properties path) handled at filter.c:240-241. |
| Blocklist append preserves the pre-filled defaults (WYSIWYG) | PASS | Type defaults are registered on the settings object (filter.c:311-320, "blocklist" default = ss_watcher_default_blocklist_text()), so obs_data_get_string returns the default text when the user never edited it; the append writes defaults + newline + proc as the explicit value -- exactly what the textbox showed plus the new line. Clearing the box still restores built-ins (watcher.h:40-42 / filter.c:113 comment -- unchanged by this diff). |
| Empty-allowlist semantics unchanged | PASS | Appending the first entry writes just the proc (filter.c:252-255); no implicit approvals are introduced anywhere; empty allowlist = approve nothing is untouched (watcher.h:50-53; no watcher list-handling changes in this diff). |
| No settings keys leak beyond "picker_window" | PASS | Buttons ("picker_refresh", "picker_add") store no value; the only new value-bearing property is the combo "picker_window" (one harmless string, accepted by the audit brief). No debug keys (M1/M2 lesson). picker_fill_combo writes properties, not settings. |

## 4. Version / package / docs

| Claim | Verdict | Evidence |
|---|---|---|
| buildspec.json version 0.2.0 | PASS | buildspec.json:41. |
| README Usage matches as-built properties + locale | PASS | Enable / "Masking mode" + both mode labels / Blocklist / Allowlist / picker strings ("Open windows", "Refresh window list", "Add selected window to the active list") / panic hotkey name all match filter.c:281-307,142 and en-US.ini:1-13 verbatim. "Duplicates are not added twice" = list_has_line; chip non-disableable = no such property exists; panic not persisted = filter.c:73-74 (memory-only); toasts/password fields active in both modes = SPEC 2.3 semantics (README:110-112). |
| Zip name matches the packaging dry-run | PASS | README:65 streamsentry-0.2.0-windows-x64.zip = release/streamsentry-0.2.0-windows-x64.zip on disk (2026-07-10 16:51). Zip verified CURRENT: packaged en-US.ini carries the 3 picker keys; packaged streamsentry.dll contains the "picker_window" literal; PERF strings ("PERF render decision", "PERF tick") absent from the DLL -> built PERF_LOG=OFF as V02-FINAL states; release/ is not whitelisted by .gitignore (ignored, as claimed). |
| Roadmap no longer lists shipped features | PASS | README:157-163 lists only unshipped items (window-capture geometry, Focus Assist, macOS, per-app policies + the never-planned list); allowlist/panic/picker removed. Consistent with SPEC v0.2 out-of-scope. |
| V02-FINAL.md claims backed by cited reports | PASS | Milestone/commit table = git log exactly (M5 a62be5a, M6 409964e, M6.5 37d8145, M7 706cda1, M8 "this commit"). All cited files exist: M5/M6/M6.5/M7 verifier + spec-guardian reports, GATE-CATCHES.md (exactly 6 cases), V02-PLAN.md, M6-toast-probe.txt, RULING + 2 addenda, TESTING.md. Final gate verdicts as claimed: M5 VERIFIED/PASS, M6 FINAL VERDICT VERIFIED (M6-verifier.md:302) / PASS, M6.5 VERIFIED runs 1-3 (M6.5-verifier.md:6) / ROUND-3 PASS (M6.5-spec-guardian.md:407) after two disclosed FAIL rounds, M7 RUN-2 VERIFIED (M7-verifier.md:322) / Round-2 PASS (M7-spec-guardian.md:408). Intermediate FAILs are not hidden -- the same paragraph points to GATE-CATCHES (cases 3, 4, 6). "55 asserts" checked: frame-decide-tests.c has 56 CHECK( lines minus 1 define = 55. "4 ctest suites + watcher-selftest incl. picker enumeration" matches tests/ on disk and the new selftest block. Open-items section is forthright (owner-manual rows, BLOCKED toast rows, untouched SPEC 2.6 -- non-gating by spec). |

## 5. Iron rules against this diff

| Rule | Verdict | Evidence |
|---|---|---|
| 1. Failure-notice integrity (as amended, RULING 2026-07-09) | PASS (no regression) | filter_video_render and frame-decide are byte-identical in this diff; the chip remains non-disableable (no new property); allowlist failure direction untouched. The picker adds no output-affecting failure path: enumeration failure/empty just yields an empty combo -- UI-only, no masking decision involved, nothing silent about protection state. |
| 2. Deterministic only | PASS | EnumWindows / GetWindowRect / GetWindowTextW / QueryFullProcessImageNameW / DwmGetWindowAttribute -- deterministic OS queries; no ML/heuristics. |
| 3. Opacity of drawn masks | NOT-APPLICABLE | No mask/plate/render changes in this change set (verified: plate-gen, frame-decide, filter render path untouched). |
| 4. No third-party dependencies | PASS | New code uses Win32 + CRT (ctype.h, stdio.h) + libobs only. No new link libraries, no Qt. |
| 5. Scope locked to v0.2 | PASS | Change set = SPEC 2.5 picker (filter.c, watcher.cpp/h, selftest) + the approved M8 docs/package/report items (README, ARCHITECTURE, buildspec, locale, V02-FINAL.md). No Chromium code (SPEC 2.6 stays documentation-only, disclosed as untouched in V02-FINAL item 5); no window-capture geometry code (roadmap text only); no backlog items. |
| 6. GPLv2 / origin | PASS | GPL headers intact; new code is straightforward Win32/libobs usage consistent with the existing codebase; no imported/adapted foreign code observed. |
| Threading contract | PASS | Enumeration cost is on the UI thread (properties dialog open / button click), one-shot; watcher tick untouched; render-side critical sections untouched; zero new locks. picker_add's obs_source_update -> filter_update -> ss_watcher_set_* is the pre-existing settings path. |

## 6. TESTING.md / scribe (audit item 6)

TESTING.md carries no M8 note and CHANGELOG.md no 0.2.0-m8 entry in the staged
diff -- consistent with the established process (the M6.5 and M7 milestone
commits show scribe-written TESTING.md + CHANGELOG.md landing together with the
gate reports). Per the M7 guardian V3 ruling, the same binding condition
attaches here:

- **Condition C1**: the scribe's M8 TESTING.md note (picker manual rows +
  the V02-FINAL open-item checklist it references) and, per the established
  pattern, the CHANGELOG 0.2.0-m8 entry MUST be in the M8 milestone commit.
- **Condition C2**: this report must be staged into the same commit (the
  V02-FINAL "every milestone gated" claim depends on it).

## 7. Violations (blocking)

None.

## 8. QUESTIONS for the human

None. (The one candidate -- the picker excludes CLOAK_UNKNOWN while allowlist
detection processes such windows -- is already answered by ruling addendum 2
item 6 read with SPEC 2.5's "UI convenience" framing: the exclusion biases
mask-more and is disclosed in ARCHITECTURE.md's M8 paragraph. Recorded as O6.)

## 9. Non-blocking observations

- O1 (pre-existing, NOT in this change set): README:66-69 install step 2's
  opening clause ("so that streamsentry.dll lands in obs-plugins\64bit\")
  describes the classic layout, while the zip -- correctly described by the
  same sentence's parenthetical -- uses the plugins-dir layout
  (streamsentry/bin/64bit + streamsentry/data). Wording accepted at the M4
  gate (M4-verifier.md:116-120; layout unchanged since); recommend a wording
  pass before the public release.
- O2: utf16_to_utf8 drops (rather than truncates) titles whose UTF-8 exceeds
  127 bytes -> such combo entries show the process name only. Disclosed by
  the struct comment ("may be empty", watcher.h:66). Cosmetic.
- O3: picker capacity is SS_MAX_RECTS (=64) distinct processes
  (filter.c:212-213); busier desktops lose later entries from the UI list
  only -- the textbox covers those. Cosmetic.
- O4: list_has_line's tolower is C-locale/ASCII; exotic non-ASCII case pairs
  could evade dedupe, producing at worst a redundant line (matching itself
  is unaffected -- the watcher lowercases via towlower). Cosmetic.
- O5: README:179 "Pure-logic modules (coordinate mapping, plate generation)
  have unit tests" understates the current four suites; the status line
  (README:9) is accurate. Cosmetic.
- O6: in allowlist mode, a CLOAK_UNKNOWN window being masked by default-deny
  will not appear in the picker (its gate is stricter), so approving it
  requires typing. Mask-more direction; consistent with ruling addendum 2
  item 6; disclosed in ARCHITECTURE.md's M8 paragraph.

## Verdict

**PASS** -- conditional on C1 and C2 (section 6): the scribe's M8 TESTING.md
note (+ CHANGELOG entry per pattern) and this report must land in the M8
milestone commit. No violations; v0.2 scope closes with this change set.
