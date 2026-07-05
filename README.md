# StreamSentry

Accident insurance for screen capture — a Windows OBS Studio video filter
that deterministically masks the classic on-stream privacy accidents
before they reach your capture.

## What it guards against

- **Notification toasts** popping up with private message content
- **Accidentally exposed sensitive windows** — password managers, system
  credential dialogs (user-editable blocklist)
- **Password fields** gaining focus — a secondary guard against "show
  password" toggles, autofill reveals, and legacy plaintext apps

## How

Detection is OS-level only: Win32 window enumeration plus UI Automation.
No machine learning, no content inspection, nothing probabilistic.

If detection health can't be verified — the watcher thread stalls,
coordinate mapping fails, anything uncertain — the filter **fails
closed**: the entire output goes black rather than risk leaking anything.
This behavior is not user-disableable.

Masking is always opaque, never blur or pixelation (archived recordings
can be deblurred offline, so reversible obfuscation isn't a real
guarantee):
- Toast rects get a notification-shaped placeholder card ("Notification
  hidden")
- Window and password-field rects get a dark privacy plate with a lock
  icon ("Hidden")

## Status

Early development (v0.1, pre-alpha). Current milestone: **M0 — plugin
skeleton**. The filter loads in OBS and passes video through untouched;
detection and masking are not implemented yet.

See [SPEC.md](SPEC.md) for the full v0.1 technical specification and
[CHANGELOG.md](CHANGELOG.md) for progress.

## Requirements

- Windows 10 21H2+ or Windows 11, x64
- OBS Studio 30+

## Building

```
cmake --preset windows-x64
cmake --build --preset windows-x64
```

Requires CMake 3.28+ and the Visual Studio 2022 C++ build tools. See
[SETUP.md](SETUP.md) for environment bootstrapping and known local-build
gotchas (e.g. Windows SDK version mismatches).

## License

GPL-2.0-or-later — see [LICENSE](LICENSE).
