# Skylanders Engine Handoff Summary

This is the current compact handoff for reviewers, resumed agents, or new contributors.

## Objective

Skylanders Engine is a Windows launcher/runtime for the Wii U Skylanders games. It should hide emulator setup, boot the selected game through Cemu, and provide an in-game Ultralight overlay for portal figure placement, controls, diagnostics, and setup.

The project does not include ROMs, keys, or user figure saves.

## Current Milestone

The default architecture is bundled external Cemu:

- Cemu is built from `extern/cemu`.
- The launcher copies/runs bundled `cemu/Cemu.exe`.
- The overlay talks to patched bundled Cemu through named pipes.
- True in-process Cemu is experimental behind `SKYLANDERS_EMBED_CEMU=ON` and currently reaches headless readiness/title-preparation diagnostics only.

This is intentional. The old "embed Cemu fully" path needs deeper Cemu bootstrap/render-host work before it can replace the external executable.

## Implemented Systems

- Six-game Wii U catalog and stub resolution.
- User-supplied ROM discovery, validation, picker, and persisted external paths.
- User-supplied `keys/keys.txt` setup.
- Bundled Cemu resolution and fallback external Cemu picker.
- Hidden Ultralight overlay boot with automatic Cemu launch.
- Character/toy catalog generated from Cemu's Skylander creation table.
- Character selector overlay with categories, game clamp, slots, place/clear, renders/icons, and diagnostics.
- Figure file paths under `figures/<GameId>/<assetKey>.sky`.
- Portal IPC request/response bridge for `place`, `place_or_create`, and `remove`.
- Controller mapping UI backed by bundled Cemu input IPC.
- Player 1 default profile and optional Player 2 profile.
- Overlay toggle config with always-on `F1`.
- Graphic-pack download/update, selection diagnostics, and non-fatal warnings.
- Per-game Cemu settings/profile preparation.
- V1.6 packaging and packaged-output smoke scripts.
- Experimental V2 backend/native-overlay scaffolding.
- V2.1 embedded headless bootstrap link/test path and diagnostics.

## Key Files

- `README.md`: setup/build/run entrypoint.
- `docs/PROJECT_MAP.md`: architecture and data flow.
- `docs/IMPLEMENTATION_STATUS.md`: current done/not-done status.
- `src/main.cpp`: Ultralight host, native bridge, setup flow, overlay lifecycle.
- `src/ProcessManager.cpp`: Cemu launch preparation.
- `src/CemuConfigEditors.cpp`: Cemu XML/INI/graphic-pack editing.
- `src/GraphicPackManager.cpp`: graphic-pack download/install state.
- `src/PortalBridge.cpp`: launcher-side portal IPC.
- `src/InputBridge.cpp`: launcher-side input IPC.
- `src/EmbeddedCemuHost.cpp`: experimental V2 embedded Cemu host.
- `src/NativeOverlayRenderer.cpp`: V2 native overlay renderer placeholder.
- `src/InputRouter.cpp`: future central input routing/suppression.
- `src/CharacterCatalog.cpp` and `src/GeneratedCharacterCatalog.inc`: Cemu toy catalog.
- `assets/app.js`, `assets/style.css`, `assets/index.html`: overlay UI.
- `tools/package_release.ps1`: creates a clean distributable folder.
- `tools/smoke_check_release.ps1`: verifies packaged output.
- `docs/V1_6_RELEASE_CHECKLIST.md`: release procedure.
- `docs/V2_NATIVE_OVERLAY_ROADMAP.md`: V2.1-V2.4 roadmap.
- `docs/V2_2_SESSION_BRIEF.md`: next-session starting brief for native host-window work.
- `extern/cemu/src/Cafe/OS/libs/nsyshid/SkylandersEnginePortalIpc.*`: Cemu-side portal bridge.
- `extern/cemu/src/Cafe/OS/libs/nsyshid/SkylandersEngineInputIpc.*`: Cemu-side input bridge.
- `extern/cemu/src/SkylandersEngineBootstrap.*`: linkable Cemu bootstrap split for V2.1.
- `extern/cemu/src/SkylandersEngineHeadlessWindowSystem.cpp`: no-GUI window-system shim for embedded readiness.

## Local Runtime State

Ignored folders/files:

- `roms/`: user game files.
- `keys/keys.txt`: user keys.
- `figures/`: generated figure files and progress.
- `runtime/config/*.json`: setup, paths, mappings, diagnostics.
- `runtime/controllerProfiles/`: Cemu controller profiles.
- `runtime/<GameId>/mlc01/`: per-game Cemu data.
- `sdks/ultralight/`: local SDK.

## Verification Commands

```powershell
node --check assets/app.js
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --build build-bundled --config Release
ctest --test-dir build-bundled -C Release --output-on-failure
cmake -S . -B build-embedded -DSKYLANDERS_EMBED_CEMU=ON -DSKYLANDERS_BUILD_BUNDLED_CEMU=ON
cmake --build build-embedded --config Release --target SkylandersEngineTests
ctest --test-dir build-embedded -C Release --output-on-failure
```

Package smoke:

```powershell
.\tools\package_release.ps1 -BuildDir build-bundled -Configuration Release
.\tools\smoke_check_release.ps1 -PackagePath out\releases\SkylandersEngine-<timestamp>
```

If MSBuild fails because this sandbox has both `Path` and `PATH`, remove the duplicate before building:

```powershell
Remove-Item Env:PATH -ErrorAction SilentlyContinue
```

## Recommended Next Step

Start a fresh session for V2.2 and use `docs/V2_2_SESSION_BRIEF.md` as the first context file. The next technical goal is launcher-owned native host-window/render ownership for `EmbeddedNative`, while bundled external Cemu remains the default fallback.

## Documentation Rule

Keep `README.md`, `docs/PROJECT_MAP.md`, `docs/IMPLEMENTATION_STATUS.md`, `docs/ConsultantHandoffSummary.md`, `docs/DOCUMENTATION_MAINTENANCE.md`, and the milestone-specific docs current with code changes. See `docs/DOCUMENTATION_MAINTENANCE.md` before committing handoff work.
