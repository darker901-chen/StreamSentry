# Owner ruling: remove the panic hotkey before beta publication

Date: 2026-07-15

## Decision

The panic hotkey was experimental test scaffolding, not a shipping user
requirement. Remove it from the v0.2 beta before public publication.

The release candidate must therefore:

- register no StreamSentry hotkey in OBS;
- retain no panic toggle or panic state in the plugin;
- contain no `PanicHotkey` locale entry;
- omit panic setup and acceptance steps from current user and publishing docs.

Historical milestone and gate reports remain unchanged as evidence of what was
implemented and tested at those earlier fingerprints. Current specifications,
architecture, release documentation, and new gate evidence supersede those
historical descriptions for the shipping candidate.

## Unchanged behavior

Allowlist mode's full-source opaque mask-all fallback remains part of v0.2. It
is deterministic default-deny behavior, not a user-triggered panic control.
Blocklist mode continues to render the source with a protection-degraded chip
when protection cannot be verified.
