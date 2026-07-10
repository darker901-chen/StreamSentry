# Owner ruling 2026-07-09 — fail-open repositioning (FINAL)

Owner ruled in session, verbatim intent (Traditional Chinese, paraphrased
faithfully):

1. **Wrong masking is always worse than under-masking.** ("亂遮一定比
   少遮問題多") Without this plugin the leak would happen anyway; a
   plugin that disrupts the user's existing workflow "只會變成個爛東西"
   (just becomes junk).
2. **Never disrupt the user's operation. This is the premise.** Masks
   may be drawn ONLY when the system is confident they are placed
   correctly ("除非有把握有作對才能遮").
3. **When protection cannot be verified, do not black out and do not
   guess-mask: keep rendering, and TELL the user the system has
   failed** ("你可以告訴使用者系統已經失效 計算失敗").
4. On the iron rules' authority: the owner states the original
   "constitution" was drafted by the setup process, not sworn by them;
   they exercise ownership now. **Decision final, no further debate
   requested** ("這不用討論了").
5. The product positioning is an **assist** ("就是輔助"), not
   insurance. The multi-agent development workflow (verifier /
   spec-guardian / scribe gates) must be written into the project
   documentation.

## Engineering translation (applied from this commit forward)

- Blocklist mode, any unverified state (stale heartbeat, dead watcher,
  unresolved capture geometry, mapping failure): render the source
  UNMODIFIED and draw a small opaque status chip ("protection
  inactive") plus OBS log lines. Full-frame blackout is REMOVED.
- Confidently detected and mapped threats are still masked (opacity
  rules unchanged for what IS drawn). Plate-texture allocation failure
  for a confident rect falls back to a solid opaque fill, never to
  dropping the mask.
- Allowlist mode (M7): failure falls back to that mode's own default —
  mask-all — because default-deny is what the user opted into; hole
  placement (approved windows) follows the same confidence rule as
  masks.
- Toast geometry gate: uncertainty now classifies as NOT-a-toast (no
  mask) instead of toast (was over-mask under the old rule 1).
  Coordinate padding on confident detections stays (placement
  tolerance, not guess-masking).
- CLAUDE.md iron rules 1 and 3 rewritten accordingly; SPEC.md Part 2
  gains §2.7 recording this repositioning; README claims updated
  minimally now, full polish at M8.

Supersedes: the fail-closed-blackout semantics of iron rule 1 (v0.1
through M6), including "raw black reserved for the fail-closed state"
in rule 3. Does NOT change: deterministic-only, opaque-masking (for
drawn masks), no-third-party-deps, GPLv2, scope lock mechanics, the
500ms heartbeat staleness definition (it now gates the failure notice
instead of a blackout).

## Application addendum (2026-07-10, after first spec-guardian audit)

The guardian's first M6.5 audit (FAIL, 5 findings) was resolved by
applying the ruling's principles to the flagged spots; recorded here so
the authority chain is explicit:

1. **No silent degradation** (guardian V1): watcher-side degradations
   that do not stall the heartbeat (monitor enumeration failure →
   toast gate cannot affirm) now publish a `detection_degraded` flag;
   the render side shows the chip and the watcher logs the transition.
2. **Chip label generalized** to "StreamSentry: protection degraded -
   see log" — some degradations leave partial masking active
   ("inactive - not masking" would overstate).
3. **Cloak-query failure direction flipped** (guardian's §8 QUESTION,
   resolved by applying the ruling's mask-only-on-confidence
   principle): a window whose DWM cloak state cannot be queried is
   treated as cloaked and skipped — a plate over a window that is not
   actually displayed would be a wrong mask. SPEC Part 1's
   report-on-doubt line carries a superseded note.
4. **The guardian's own checklist** (.claude/agents/spec-guardian.md)
   was amended under this ruling's authority (guardian V5): the
   "failure paths must land in blackout" bullet is replaced by
   failure-notice integrity per the amended rule 1.
