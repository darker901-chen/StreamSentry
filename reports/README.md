# reports/ — internal milestone evidence (frozen)

Nothing in this folder is needed to use or build the plugin. It is the
paper trail of the project's gate workflow: every milestone must pass
an independent build-and-test verification and a governance audit
before it lands, and those reports are committed here verbatim and
never edited afterwards.

| File pattern | What it is |
|---|---|
| `M<n>-verifier.md` | Build + automated-test evidence for milestone n (verdict VERIFIED/FAILED) |
| `M<n>-spec-guardian.md` | Governance audit of milestone n against CLAUDE.md / SPEC.md (verdict PASS/FAIL) |
| `M2-DECISIONS.md` | Decision log — reconciliations between spec and as-built, with owner rulings |
| `M3-perf.md`, `M3-soak-samples.csv`, `M3-acceptance-matrix.md` | Performance / endurance / acceptance measurements for v0.1 |
| `M2-*.txt` | Raw captured output backing the M2 reports |
| `FINAL.md` | v0.1 release report |
| `HUMAN_CHECKLIST.md` | The owner's manual in-OBS verification pass |
| `V02-PLAN.md` | Owner-approved v0.2 plan and the field-test findings that drove it |
| `RULING-*.md` | Owner rulings that amend the project's rules (binding on all later audits) |
| `GATE-CATCHES.md` | Index of real defects the gate agents caught, with the story of each (Traditional Chinese) |

If you are curious about the project's quality story, read `FINAL.md`
first, then the newest `M<n>-*` pair.
