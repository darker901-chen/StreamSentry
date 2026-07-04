# M0 Verification Report — obsplugin

- Date: 2026-07-05
- Verifier: automated evidence collection (Claude Code verifier agent)
- Scope: Milestone M0 — skeleton video filter from obs-plugintemplate. Pass-through only;
  no detection, no masking, no fail-closed rendering (those are M1+).
- Note: `build_x64/` reused the existing configure cache (`.deps` already populated).
  Configure was NOT re-run, per instructions; only the build step was executed.

---

## Check 1 — Build succeeds, artifact exists: PASS

Command (Git Bash, from `F:\obsplugin`):

    cmake --build --preset windows-x64-local > build.log 2>&1; echo "BUILD EXIT CODE: $?"

Output:

    BUILD EXIT CODE: 0
    .NET Framework 的 MSBuild 版本 17.14.40+3e7442088
      plugin-support.vcxproj -> F:\obsplugin\build_x64\RelWithDebInfo\plugin-support.lib
      obsplugin.vcxproj -> F:\obsplugin\build_x64\RelWithDebInfo\obsplugin.dll
      Copy obsplugin to rundir	Copy obsplugin resources to rundir

Warnings/errors scan: `grep -inE 'warning|error' build.log` → no matches. Zero warnings accepted silently; there were none.

Artifact:

    ls -la F:/obsplugin/build_x64/RelWithDebInfo/obsplugin.dll
    -rwxr-xr-x 1 darker 197610 14848 Jul  5 02:02 F:/obsplugin/build_x64/RelWithDebInfo/obsplugin.dll

The build was an incremental no-op (targets already up to date): artifact mtime and SHA256
are identical pre- and post-build (hash evidence in Check 5).

## Check 2 — Import table (dumpbin /dependents): PASS

Command (run with `MSYS_NO_PATHCONV=1` to stop Git Bash mangling `/dependents`):

    C:\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe /dependents F:\obsplugin\build_x64\RelWithDebInfo\obsplugin.dll

Output (full dependency list):

    obs.dll
    VCRUNTIME140.dll
    api-ms-win-crt-stdio-l1-1-0.dll
    api-ms-win-crt-heap-l1-1-0.dll
    api-ms-win-crt-runtime-l1-1-0.dll
    KERNEL32.dll

Contains obs.dll; every other entry is VCRUNTIME/api-ms-win-crt*/KERNEL32 (all on the
acceptable list). No Qt6*, no third-party DLLs.

## Check 3 — Export table (dumpbin /exports): PASS

Command:

    C:\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe /exports F:\obsplugin\build_x64\RelWithDebInfo\obsplugin.dll

Output (7 exports — standard OBS module ABI):

    1  obs_module_free_locale
    2  obs_module_get_string
    3  obs_module_load          <-- required export present
    4  obs_module_set_locale
    5  obs_module_set_pointer
    6  obs_module_unload
    7  obs_module_ver

## Check 4 — Source audit: PASS

`src/` tree contains exactly: `plugin-main.c`, `plugin-support.h`, `plugin-support.c.in`.

Read of `F:\obsplugin\src\plugin-main.c` confirms:

- Exactly one source registered: `obs_module_load()` calls `obs_register_source(&obsplugin_filter_info)` once.
- `obsplugin_filter_info`: `.id = "obsplugin_filter"`, `.type = OBS_SOURCE_TYPE_FILTER`,
  `.output_flags = OBS_SOURCE_VIDEO`.
- Display name: `obs_module_text("FilterName")`; `data/locale/en-US.ini` line 1:
  `FilterName="obsplugin"` → filter is named "obsplugin".
- Properties: `obsplugin_filter_get_properties()` adds exactly one property:
  `obs_properties_add_bool(props, "enabled", obs_module_text("Enable"))`. Nothing else.
  (Defaults set `enabled = true` via `obs_data_set_default_bool` — a default, not an extra property.)
- `obsplugin_filter_video_render()` calls `obs_source_skip_video_filter(filter->source)` — pure pass-through, no drawing.

Forbidden-API grep over the whole `src/` tree (ripgrep, case-insensitive):

    pattern: EnumWindows|SetWinEventHook|IUIAutomation|CreateThread|gs_draw|blur|pixelate|mosaic|SetWindowsHookEx|_beginthread|AccessibleObjectFromWindow
    path:    F:\obsplugin\src
    result:  No matches found

`CMakeLists.txt`: `ENABLE_FRONTEND_API OFF`, `ENABLE_QT OFF`; only link target is
`OBS::libobs`. No Qt/frontend/third-party dependency is configured.

## Check 5 — Deployment consistency (SHA256): PASS

Command:

    sha256sum F:/obsplugin/build_x64/RelWithDebInfo/obsplugin.dll D:/software/obs/obs-studio/obs-plugins/64bit/obsplugin.dll F:/obsplugin/data/locale/en-US.ini "D:/software/obs/obs-studio/data/obs-plugins/obsplugin/locale/en-US.ini"

Results (measured both before and after the build in Check 1 — identical both times,
i.e. the incremental build did not relink and no previous-artifact comparison was needed):

    2f02ff771c61ef200682d5087e83165e4889b5dd145bd216dade0264766c08f0  F:/obsplugin/build_x64/RelWithDebInfo/obsplugin.dll
    2f02ff771c61ef200682d5087e83165e4889b5dd145bd216dade0264766c08f0  D:/software/obs/obs-studio/obs-plugins/64bit/obsplugin.dll
    44d046cf67f573ef0b01ecb1233c8e3569ed89dc003fb7102d19c23d7c61888d  F:/obsplugin/data/locale/en-US.ini
    44d046cf67f573ef0b01ecb1233c8e3569ed89dc003fb7102d19c23d7c61888d  D:/software/obs/obs-studio/data/obs-plugins/obsplugin/locale/en-US.ini

Build artifact == deployed DLL; repo locale file == deployed locale file.

## Check 6 — Git working tree: PASS

Commands and output:

    git -C F:/obsplugin status --short
    (empty output, exit 0 — no tracked modifications, no unignored untracked files)

    git -C F:/obsplugin check-ignore -v CMakeUserPresets.json build_x64 .deps
    .gitignore:2:/*   CMakeUserPresets.json
    .gitignore:2:/*   build_x64
    .gitignore:2:/*   .deps

    git -C F:/obsplugin log --oneline -3
    1c0b2f0 Add minimal pass-through video filter (M0)
    7db2454 Track project governance files alongside template whitelist gitignore
    eb47d8b Initialize plugin skeleton from obs-plugintemplate

`CMakeUserPresets.json` and `build_x64/` are ignored (whitelist-style `.gitignore`) and
absent from status. `git ls-files` confirms `src/`, `data/locale/en-US.ini`, CMake files,
SPEC.md, CLAUDE.md etc. are tracked.

## Not covered by automation

- No automated test suite exists at M0 (no ctest targets, no test executables); nothing to run.
- Actually loading the DLL inside OBS Studio (module load log line, filter appearing in the
  filter list as "obsplugin", the Enable checkbox rendering in the properties dialog, and
  visual confirmation of pass-through output) requires a manual in-OBS check.
- Future in-OBS behavior (toast masking, fail-closed blackout) is M1+ and requires the
  manual test matrix in SPEC.md; explicitly out of scope for M0.

## Verdict

VERDICT: VERIFIED
