# HUMAN_CHECKLIST — items automation cannot verify

Accumulated across milestones; do these in one manual pass near the end.
Everything here has programmatic evidence for the underlying mechanism —
these entries are only about what a human eye must confirm on screen.

Recommended test source: add a **Display Capture** (監視器擷取) source at
native resolution (geometry resolution in v0.1 supports display capture;
see reports/M2-DECISIONS.md), then add the **StreamSentry** filter to it.

---

## M1 plate visuals — now verified via real M2 detection

The M1 fake-rect DEBUG toggles were removed in M2 (real detection
replaced them). The plate *rendering* is unchanged, so the M1 visual
check folds into the M2 checks below: confirm the toast card and privacy
plate look right there.

## M2 — real detection & masking (~8 min; turn OFF Do Not Disturb first)

Do Not Disturb / Focus Assist suppresses toast banners — turn it OFF in
Windows Settings → System → Notifications before the toast step, or the
toast will never appear on screen (this is why the automated toast test
reports INCONCLUSIVE).

1. [ ] **Password field (UIA, on by default).** With a Display Capture
       filtered by StreamSentry, open a login page in a browser and click
       the password field. A dark **privacy plate** (lock icon + "Hidden")
       must cover the field within ~1-2 frames. Click away → it clears.
2. [ ] **Toast card.** With DND off, trigger a real notification (send
       yourself a LINE/Slack/Teams message, or run the toast snippet in
       reports/M2-DECISIONS.md). A rounded **notification card** (bell +
       "Notification hidden") must cover the toast before its text is
       readable. Known over-mask: Start-menu / taskbar XAML popups may
       also get a card — expected (iron rule 1), documented.
3. [ ] **Blocklist window.** The built-in default blocklist is password
       managers + credential dialogs (1Password, KeePass, Bitwarden,
       consent.exe, logonui, credentialuibroker). If you have one, open it
       over the captured screen → full-window **privacy plate**. (Custom
       blocklist entries via the settings textbox arrive in M3; until then
       the automated selftest proves arbitrary-name matching using
       notepad — see reports/M2-watcher-selftest-output.txt.)
4. [ ] **Opacity.** Confirm nothing of the underlying content shows through
       any plate/card (unit tests assert alpha=255; this is the on-screen
       confirmation).
5. [ ] **Fail-closed blackout.** In the filter's Developer group, tick
       "DEBUG: freeze watcher heartbeat". Within ~0.5s the WHOLE source
       goes black with the red "Privacy guard: detection unavailable -
       output blocked" banner. Untick → normal within ~0.5s. (Log
       evidence already captured; this is the eyeball confirmation.)
6. [ ] **Window capture caveat.** If you attach StreamSentry to a *Window*
       Capture (not Display Capture) and any sensitive window is detected,
       the source goes fully black — v0.1 only resolves display-capture
       geometry and fails closed otherwise (reports/M2-DECISIONS.md). Note
       whether this matches your expectation.

When done, switch scene collection back to "無標題".

## Flagged for your ruling (see reports/M2-DECISIONS.md)

- [ ] **Blocklist match semantics.** CLAUDE.md says "process AND class";
      SPEC says "process names / title substrings". For a blocklist,
      requiring both risks *under*-masking (iron rule 1 violation), so M2
      matches process-name-OR-title-substring. Confirm this is what you
      want, or ask for strict AND-semantics.
