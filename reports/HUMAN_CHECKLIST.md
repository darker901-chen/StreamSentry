# HUMAN_CHECKLIST — items automation cannot verify

One manual pass before release. Every item has programmatic evidence for
the underlying mechanism (see reports/); these are the on-screen / real-
device confirmations a human must still make. Cross-referenced to the
SPEC.md acceptance matrix rows (reports/M3-acceptance-matrix.md).

Setup: add a **Display Capture** (螢幕擷取) source at native resolution,
then add the **StreamSentry** filter to it. Turn **Do Not Disturb OFF**
(Windows Settings → System → Notifications) before the toast step.

## Settings UI (matrix rows N/A — UI check)
1. [ ] Filter properties show exactly: an **Enable** checkbox and a
       multi-line **Blocklist** text box pre-filled with the default
       entries (1password, keepass, bitwarden, dashlane, lastpass,
       consent.exe, credentialuibroker, logonui.exe). There is **no**
       option to disable fail-closed. Editing a line and confirming takes
       effect (add e.g. "notepad", open Notepad over the captured screen →
       it gets a privacy plate; remove it → no longer masked).

## Real detection & masking
2. [ ] **Password field (row 2).** Open a login page, click the password
       field → dark **privacy plate** (lock + "Hidden") covers it within
       1–2 frames; click away → clears.
3. [ ] **Toast (row 1, DND off).** Trigger a real notification → rounded
       **notification card** (bell + "Notification hidden") covers it
       before the text is readable. Known over-mask: Start-menu / taskbar
       XAML popups may also get a card (iron rule 1, documented).
4. [ ] **Blocklist window (row 4).** Open a listed app (1Password/KeePass
       if installed, or add any app's name to the box) over the captured
       screen → full-window **privacy plate**.
5. [ ] **Opacity.** Nothing of the underlying content shows through any
       plate/card (unit tests assert alpha=255; this confirms on screen).

## Coordinate accuracy (rows 5, 6)
6. [ ] **Second monitor / different DPI.** Move a password field to a 2nd
       monitor at a different scale → plate still lands on it. (Note: two
       monitors of *identical* resolution are not distinguishable in v0.1
       and fail closed by design — reports/M2-DECISIONS.md.)
7. [ ] **Scale/crop transform.** Apply a scale or crop to the captured
       source. NOTE: v0.1 geometry resolution only supports an unscaled
       full-monitor Display Capture; a scaled capture **fails closed**
       (black) rather than mis-place a mask. Confirm this matches your
       expectation, or flag it for a later refinement.

## Fail-closed (row 7 — already machine-verified)
8. [ ] The ≤500ms blackout + red "detection unavailable" banner is
       machine-verified (reports/M2-obs-integration.txt, watcher-selftest,
       ~517ms). The shipping build has no debug trigger (scaffolding
       removed). To see it live: attach StreamSentry to a **Window
       Capture** (not Display Capture) and open any blocklisted window —
       window-capture geometry is unsupported in v0.1, so the source goes
       fully black. Confirm the black + banner appears.

## Endurance (row 9)
9. [ ] A 30-minute soak was run automatically (reports/M3-perf.md,
       M3-soak-samples.csv — working set stable). SPEC's target is a
       **2-hour** run: if you want the full guarantee, leave OBS running
       with the filter for 2h and confirm the working set does not climb.

## Decision — RESOLVED (owner ruling 2026-07-05)
10. [x] **Blocklist match semantics.** RULED: keep process-name **OR**
        title-substring (as implemented). No change. Done.

When done, switch scene collection back to "無標題".
