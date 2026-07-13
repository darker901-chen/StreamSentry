# Owner ruling 2026-07-13 — deterministic non-content window exclusions (FINAL)

This ruling formalizes the field fixes in commits `b14b5e2` and `2023587`.
It amends SPEC.md §2.3's earlier statement that every shell surface starts
masked in allowlist mode.

## Decision

StreamSentry does not report these known non-content top-level windows as mask
rectangles in either matching mode:

1. Windows whose width or height is at most 16 physical pixels. These are
   resize borders and drop-shadow strips and cannot contain readable private
   content at a supported DPI.
2. Windows with class `Progman` or `WorkerW`. These are Windows desktop
   wallpaper hosts. Masking their full-screen rectangles makes allowlist mode
   unusable because the rectangle covers approved windows above the desktop.

The taskbar (`Shell_TrayWnd`) and real application windows are not exempt.
Toast signatures and UIA password-field rectangles remain subject to their
existing rules. The exclusions are based only on deterministic geometry and
window class; they do not add process approvals or content inference.

## Product wording

"Allowlist" means mask every detectable content-bearing window except an
approved one. An empty allowlist approves no application or taskbar window,
but the desktop wallpaper remains visible. Panic and every unverified
allowlist failure path still use the full-source mask-all plate.

## Acceptance consequence

The two post-M8 field-fix commits require a fresh verifier report, spec audit,
and scribe pass together. Older M8 gate reports do not certify this amended
behavior.
