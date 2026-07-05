# M2 design decisions & a flagged CLAUDE.md/SPEC.md conflict

## FLAGGED CONFLICT — blocklist matching semantics (needs human ruling)

- **CLAUDE.md (Architecture)** says the enumeration signature for *both*
  toasts *and* blocklist is "**process name AND window class**, both must
  match".
- **SPEC.md (Detection §2)** says the blocklist is a user-editable list of
  "**process names / window-title substrings**" — i.e. match on process
  name *or* a title substring, no window-class requirement.

These conflict for the blocklist. For a **toast** the AND-of-process-and-class
rule is an anti-false-positive guard and is fine. For a **blocklist** —
whose entire job is to guarantee a sensitive window (1Password, a
credential dialog) never shows — requiring *both* process AND an exact
window class makes a miss (under-mask) far more likely, which directly
violates **iron rule 1** (any uncertainty → over-mask, never under-mask).

**Decision taken for M2 (reversible):** implement the blocklist the SPEC
way and the iron-rule-1-safe way — an entry masks a window if the process
image name matches the entry **OR** the window title contains the entry
substring (both case-insensitive). Toast detection keeps process-AND-class.

This is surfaced to the human on the checklist. If the owner wants strict
CLAUDE.md AND-semantics for the blocklist, it is a one-line change — but it
trades away masking safety, so it should be a conscious choice.

## Toast signature (determined empirically on this machine)

Windows 11 build 26200 (25H2): a toast banner is hosted by **explorer.exe**,
window class **`Xaml_WindowedPopupClass`** (title "快顯主機"). SPEC's example
(`ShellExperienceHost`-hosted CoreWindow) is Win10 21H2-era and does NOT
match here. Documented in watcher code comments.

Caveat: `Xaml_WindowedPopupClass` in explorer is also used by other XAML
flyouts (Start search, taskbar popups). Matching process+class therefore
**over-masks** those too — acceptable under iron rule 1, documented as a
known limitation. Phantom (0×0) popup windows are filtered by requiring a
visible, non-cloaked, non-empty rect, which cannot cause a real toast to be
missed.

## Do Not Disturb interferes with the automated toast test

This machine has DND/quiet-hours active; programmatic toasts are routed to
the notification center with no on-screen banner (rect stays 0×0). The
automated M2 toast test therefore reports INCONCLUSIVE (not fail) when no
banner ever gets a real rect, and the real on-screen toast-masking check is
on the human checklist with "turn off Do Not Disturb first" as a
precondition.

## Capture-geometry scope for M2

Only **display/monitor capture** geometry is resolved (monitor → virtual-
screen rect, source base size). For window capture, game capture, or any
source whose captured region cannot be confidently resolved, the filter
**fails closed** (black) when there are rects to mask, rather than guess and
mis-place a plate. This obeys iron rule 1 and is documented as a limitation
to refine later. The automated watcher tests do not depend on geometry.
