# StreamSentry installation and first-run SOP

This guide installs the Windows x64 **0.2.0 beta** and proves that OBS loaded
the expected plugin before you rely on it. StreamSentry is a privacy assist,
not a guarantee; read the limitations in the main README before streaming.

## 1. Choose the correct download

1. Open the project [Releases page](https://github.com/darker901-chen/StreamSentry/releases).
2. Open the latest **0.2.0 beta** prerelease.
3. Download `streamsentry-0.2.0-windows-x64.zip` from **Assets**.
4. Do not download GitHub's automatic `Source code (zip)` or
   `Source code (tar.gz)` files. Those contain source files, not the compiled
   OBS plugin.
5. Compare the downloaded file's SHA-256 value with the value in the release
   notes:

   ```powershell
   Get-FileHash "$env:USERPROFILE\Downloads\streamsentry-0.2.0-windows-x64.zip" -Algorithm SHA256
   ```

   Continue only if the values match. The beta DLL is unsigned, so Windows or
   security software may warn about it.

## 2. Close OBS

Exit OBS before copying or replacing the plugin. In Task Manager, confirm
`obs64.exe` is no longer running if OBS was updating or appeared to stay open.

## 3. Install it

### Standard OBS installer — recommended

1. Press **Win+R**.
2. Paste `%ProgramData%\obs-studio\plugins` and press Enter.
3. If the `plugins` folder does not exist, create it.
4. Open the downloaded zip.
5. Copy the single `streamsentry` folder into the `plugins` folder.
6. Confirm both of these files exist:

   ```text
   C:\ProgramData\obs-studio\plugins\streamsentry\bin\64bit\streamsentry.dll
   C:\ProgramData\obs-studio\plugins\streamsentry\data\locale\en-US.ini
   ```

The most common mistake is an extra directory level such as
`streamsentry\streamsentry\bin`. The first `streamsentry` folder under
`plugins` must contain `bin` and `data` directly.

### Custom-location or portable OBS

Try the recommended `%ProgramData%` layout first unless your OBS package
explicitly uses portable plugin paths. If OBS does not load the plugin:

1. In OBS, open **Help → Log Files → View Current Log** and note the OBS
   executable directory near the start of the log.
2. Use the plugin directories belonging to that custom or portable OBS copy.
   For the traditional two-directory layout, copy:

   ```text
   streamsentry\bin\64bit\streamsentry.dll
       → <OBS folder>\obs-plugins\64bit\streamsentry.dll

   streamsentry\data\locale\en-US.ini
       → <OBS folder>\data\obs-plugins\streamsentry\locale\en-US.ini
   ```

3. Do not leave different StreamSentry DLL versions in both ProgramData and
   the OBS application directory. If troubleshooting requires the traditional
   layout, keep only the copy that the current OBS log reports loading.

The application-directory layout is an exception for custom/portable setups,
not the recommended layout for a normal OBS installation.

## 4. Confirm OBS loaded StreamSentry

1. Start OBS.
2. Open **Help → Log Files → View Current Log**.
3. Search for `streamsentry`.
4. Confirm this line is present:

   ```text
   [streamsentry] plugin loaded successfully (version 0.2.0)
   ```

`Failed to load 'zh-TW' text` followed by the successful-load line only means
the beta fell back to its bundled English locale.

If the successful-load line is absent, stop here and use the troubleshooting
section. Do not assume the filter is protecting a stream merely because files
were copied.

## 5. Add the filter

1. Add or select an unscaled, full-monitor **Display Capture** source.
2. Right-click that source and choose **Filters**.
3. Under **Effect Filters**, press **+**.
4. Select **StreamSentry** and accept the default name.
5. Keep **Masking mode** on **Blocklist** for the first test.

Window Capture, cropped/scaled Display Capture, and some ambiguous
multi-monitor layouts are not supported in 0.2.0. When a mask cannot be mapped
confidently, **Blocklist** mode keeps rendering the source with a
**protection degraded** chip. **Allowlist** mode instead draws a full-source
opaque privacy plate plus the chip because its selected behavior is
default-deny.

## 6. Two-minute smoke test

Do this before the first real stream:

1. Open Notepad on the captured monitor.
2. Open the StreamSentry filter settings.
3. Under **Open windows**, select the Notepad entry and press
   **Add to list**.
4. Confirm a solid opaque **Hidden** plate covers Notepad in the OBS preview.
5. Remove the Notepad line from the blocklist and confirm the plate disappears.
6. Close Notepad and inspect the current OBS log for unexpected
   `protection degraded` or module-load errors.

This smoke test proves plugin loading, filter creation, picker-to-blocklist
routing, and opaque masking on this OBS installation. It does not prove every
toast, password field, mixed-DPI layout, or application UI.

## 7. Update

1. Close OBS.
2. Remove the existing StreamSentry plugin files from the location the OBS log
   says it loaded.
3. Install the new release using the same layout.
4. Start OBS and confirm the new version in the successful-load log line.
5. Repeat the smoke test.

OBS scene collections retain the filter settings. Keep a backup of important
scene collections before testing beta updates.

## 8. Uninstall

Close OBS, then delete the StreamSentry files from the location used during
installation:

```text
C:\ProgramData\obs-studio\plugins\streamsentry\
```

For the traditional custom/portable layout, remove both
`obs-plugins\64bit\streamsentry.dll` and
`data\obs-plugins\streamsentry\`. Start OBS again. Existing scenes may retain
an unavailable-filter entry until it is removed from that source.

## 9. Troubleshooting checklist

### StreamSentry is missing from Effect Filters

- Confirm Windows x64 and OBS Studio 32.1.2. Older releases are not part of the
  current compatibility evidence.
- Confirm the filter is being added to a **Display Capture**, not at scene level
  or under Audio Filters.
- Check for the accidental `streamsentry\streamsentry` nesting mistake.
- Search the current OBS log for `streamsentry`, `module-load`, or
  `Failed to load`.
- If multiple copies exist, remove the older copy and keep only the path the
  current OBS installation uses.
- Re-download the release asset and verify its checksum.

### The filter appears but shows “protection degraded”

- Use an unscaled full-monitor Display Capture.
- Remove crop/scale transforms during the test.
- On identical-resolution multi-monitor setups, test one monitor at a time.
- Check the OBS log for the specific geometry, heartbeat, or watcher reason.

### A notification or application popup was not masked

Some applications draw their own popups instead of Windows notification
toasts. Add the application through the blocklist picker, or use allowlist mode
for a stricter default-deny setup. Review the complete limitations in the main
README before relying on the beta.

## 10. Issue report

Open a [GitHub issue](https://github.com/darker901-chen/StreamSentry/issues) and
include:

- OBS version and Windows version;
- standard, custom-location, or portable OBS installation;
- capture-source type and monitor layout;
- exact reproduction steps;
- the StreamSentry-related log lines.

Remove stream keys, account names, message content, and other private data from
logs before attaching them.
