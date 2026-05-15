# Skylanders Engine Handoff Summary

This is the current compact handoff for reviewers, resumed agents, or new contributors.

## Objective

Skylanders Engine is a Windows launcher/runtime for the Wii U Skylanders games. It should hide emulator setup, boot the selected game through Cemu, and provide an in-game Ultralight overlay for portal figure placement, controls, diagnostics, and setup.

The project does not include ROMs, keys, or user figure saves.

## Current Milestone

The active architecture is bundled external Cemu:

- Cemu is built from `extern/cemu`.
- The launcher copies/runs bundled `cemu/Cemu.exe`.
- The overlay talks to patched bundled Cemu through named pipes.
- True in-process Cemu is deferred behind `SKYLANDERS_EMBED_CEMU=OFF`.

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
- `src/CharacterCatalog.cpp` and `src/GeneratedCharacterCatalog.inc`: Cemu toy catalog.
- `assets/app.js`, `assets/style.css`, `assets/index.html`: overlay UI.
- `extern/cemu/src/Cafe/OS/libs/nsyshid/SkylandersEnginePortalIpc.*`: Cemu-side portal bridge.
- `extern/cemu/src/Cafe/OS/libs/nsyshid/SkylandersEngineInputIpc.*`: Cemu-side input bridge.

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
```

If MSBuild fails because this sandbox has both `Path` and `PATH`, remove the duplicate before building:

```powershell
Remove-Item Env:PATH -ErrorAction SilentlyContinue
```

## Recommended Next Step

Stabilize controller mapping and input capture across SDL/DualSense devices, then verify figure persistence end to end with bundled Cemu. Those are the two biggest user-visible risks left before packaging work.

## Documentation Rule

Keep `README.md`, `docs/PROJECT_MAP.md`, `docs/IMPLEMENTATION_STATUS.md`, and `assets/README.md` current with code changes. See `docs/DOCUMENTATION_MAINTENANCE.md` before committing handoff work.
