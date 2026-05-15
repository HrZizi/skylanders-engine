# Project Map

This document maps the main systems so a new contributor can find the right code quickly.

## Runtime Boot Flow

1. A game-specific stub such as `Giants.exe` starts `src/main.cpp`.
2. `GameCatalog` resolves the stub name into a `GameId` and accepted Wii U title IDs.
3. `CemuLocator` resolves the emulator backend, preferring bundled `build/Release/cemu/Cemu.exe`.
4. `RomLocator` scans `roms/`, validates candidates, then falls back to persisted external paths or a native file picker.
5. `main.cpp` validates `keys/keys.txt` and starts a hidden Ultralight host window.
6. `ProcessManager` prepares Cemu settings, profiles, MLC folders, graphic packs, and launch diagnostics.
7. Cemu starts as an external process. The overlay remains hidden until `Start+Back`, `F1`, or a configured toggle opens it.

## Core Native Modules

### `GameCatalog`

Files:

- `include/skylanders/GameCatalog.h`
- `src/GameCatalog.cpp`

Owns supported game metadata: `GameId`, display name, stub names, accepted base title IDs, search terms, and runtime folder names.

### `RomLocator`

Files:

- `include/skylanders/RomLocator.h`
- `src/RomLocator.cpp`

Finds and validates user-supplied game files. It scans `roms/` first, then checks `runtime/config/game_paths.json`, then prompts with a Windows file picker.

Supported forms include `.wud`, `.wux`, `.wua`, `.iso`, `.rpx`, `title.tmd`, and extracted titles with `code/content/meta`.

### `CemuLocator`

Files:

- `include/skylanders/CemuLocator.h`
- `src/CemuLocator.cpp`

Resolves Cemu in this order:

1. bundled submodule output beside the stubs,
2. persisted external Cemu from `runtime/config/setup.json`,
3. portable `cemu/Cemu.exe`,
4. legacy fallback path,
5. native file picker.

Only user-selected external paths are persisted.

### `ProcessManager`

Files:

- `include/skylanders/ProcessManager.h`
- `src/ProcessManager.cpp`

Prepares and launches external Cemu. It writes global `settings.xml`, per-title `gameProfiles/<titleId>.ini`, controller profile references, per-game MLC paths, fullscreen/windowed mode, audio defaults, and graphic-pack selections. It returns a `CemuPreparationReport` and persists the last launch diagnostics.

### `CemuConfigEditors`

Files:

- `include/skylanders/CemuConfigEditors.h`
- `src/CemuConfigEditors.cpp`

Small XML/INI editor helpers for Cemu settings and game profiles. It also scans `graphicPacks/` folders and selects matching rules/presets.

Cemu graphic-pack presets are written in Cemu's nested format:

```xml
<Preset>
  <category>Resolution</category>
  <preset>2560x1440</preset>
</Preset>
```

### `GraphicPackManager`

Files:

- `include/skylanders/GraphicPackManager.h`
- `src/GraphicPackManager.cpp`

Downloads the version-6 graphic-pack ZIP, caches it under `runtime/downloads/`, extracts it into `cemuRoot/graphicPacks`, and records status in `runtime/config/graphic_packs.json`. Failures are warnings, never launch blockers.

### `CharacterCatalog`

Files:

- `include/skylanders/CharacterCatalog.h`
- `src/CharacterCatalog.cpp`
- `src/GeneratedCharacterCatalog.inc`
- `docs/cemu-skylanders-catalog.json`

Defines every Cemu-creatable toy entry, including characters, variants, SWAP halves, items, traps, vehicles, and trophies. Frontend IDs use stable `assetKey` values rather than display-name text.

### `FigureStore`

Files:

- `include/skylanders/FigureStore.h`
- `src/FigureStore.cpp`

Builds figure paths under `figures/<GameId>/<assetKey>.sky` and validates existing figure IDs when the launcher can inspect them.

### `PortalBridge`

Files:

- `include/skylanders/PortalBridge.h`
- `src/PortalBridge.cpp`

Launcher-side named-pipe client for portal actions:

- `ping`
- `place`
- `place_or_create`
- `remove`

The bridge sends JSON requests to bundled Cemu and waits for JSON success/failure responses. UI state changes only after Cemu confirms.

### `InputBridge`

Files:

- `include/skylanders/InputBridge.h`
- `src/InputBridge.cpp`

Launcher-side named-pipe client for Cemu-backed input device scanning, profile loading, and capture. The frontend uses session-based capture (`start_capture`, `poll_capture`, `cancel_capture`) so mapping does not freeze the overlay.

### `EmulatorCore`

Files:

- `include/skylanders/EmulatorCore.h`
- `src/EmulatorCore.cpp`

Reserved adapter for true in-process Cemu. `SKYLANDERS_EMBED_CEMU` remains off by default; the project currently uses bundled external Cemu.

## Cemu Submodule Patch Points

Submodule:

- `extern/cemu/`

Important patched areas:

- `src/Cafe/OS/libs/nsyshid/SkylandersEnginePortalIpc.*`
- `src/Cafe/OS/libs/nsyshid/SkylandersEngineInputIpc.*`
- `src/Cafe/OS/libs/nsyshid/Skylander.*`
- `src/Cafe/OS/libs/nsyshid/nsyshid.cpp`
- `src/input/InputManager.cpp`

These provide the bundled-only portal and input IPC used by the overlay.

## Frontend Overlay

Files:

- `assets/index.html`
- `assets/style.css`
- `assets/app.js`

Implemented views:

- character selector,
- category selector,
- portal slot placement/clear controls,
- controls mapping,
- graphic-pack and Cemu preparation diagnostics.

The overlay is prewarmed while hidden so first open is responsive. It should not be treated as the boot entrypoint.

## Runtime Files

Ignored local state:

- `runtime/config/setup.json`: external Cemu path and setup settings.
- `runtime/config/game_paths.json`: external ROM paths.
- `runtime/config/input_mapping.json`: controller profile UI state and overlay toggles.
- `runtime/config/graphic_packs.json`: last graphic-pack update status.
- `runtime/config/last_launch_diagnostics.json`: most recent Cemu preparation report.
- `runtime/controllerProfiles/`: generated Cemu controller profiles.
- `runtime/<GameId>/mlc01/`: per-game Cemu MLC folders.
- `figures/`: generated `.sky` figure files.

Tracked placeholders:

- `roms/README.md`
- `keys/README.md`
- `figures/README.md`

## Asset Pipeline

Tracked approved assets live under:

- `assets/backgrounds/`
- `assets/game-logos/`
- `assets/characters/renders/<GameId>/`
- `assets/characters/icons/<GameId>/`
- `assets/elements/`

Ignored review candidates live under `_review` folders. See [assets/README.md](../assets/README.md) for importer commands and naming rules.
