# ProcessManager Architecture

`ProcessManager` is the launcher boundary that prepares Cemu and starts the selected Wii U title. It is used by the current bundled-external-Cemu milestone.

## Responsibilities

`ProcessManager` owns launch preparation for a resolved `GameLaunchSpec`:

- create per-game runtime folders under `runtime/<GameId>/`,
- create/use per-game MLC paths,
- write Cemu global settings,
- write game profile overrides,
- wire controller profiles,
- select graphic packs and presets,
- persist launch diagnostics,
- spawn `Cemu.exe`.

The launch input is already resolved before this point:

- `GameCatalog` resolves the stub/game.
- `RomLocator` resolves and validates the ROM.
- `CemuLocator` resolves the Cemu executable/root.
- `main.cpp` validates keys and creates the hidden overlay.

## Cemu Settings

The launcher keeps conservative defaults:

- Vulkan renderer,
- async shader compilation,
- accurate Vulkan barriers,
- GX2Drawdone sync,
- Skylander portal emulation,
- isolated per-game MLC path,
- isolated controller profiles,
- conservative XAudio2/default-device audio values when no explicit user audio device is configured.

Fullscreen is opt-in through setup config. Windowed remains the default because the external Ultralight overlay is more reliable over windowed Cemu.

## Graphic Packs

`ProcessManager` uses `CemuConfigEditors` to scan `cemuRoot/graphicPacks` and enable the best matching packs.

Every attempt produces a `GraphicPackSelectionReport`:

- feature name,
- selected pack path,
- preset category/preset when applicable,
- required vs optional,
- warning/error text.

These reports are included in `CemuPreparationReport`, exposed through `nativeStateJson`, and persisted to `runtime/config/last_launch_diagnostics.json`.

Graphic-pack presets must use Cemu's nested XML element format:

```xml
<Preset>
  <category>Resolution</category>
  <preset>2560x1440</preset>
</Preset>
```

## Game Profiles

The launcher writes game profiles under Cemu's `gameProfiles/` folder using the selected title ID. Profiles are used for graphics API and controller profile assignment.

Controller profile names:

- `controller1=skylanders_player1`
- `controller2=skylanders_player2` only when Player 2 is enabled.

## Launch Command

Cemu is launched with:

```text
Cemu.exe -g "<resolved game path>" -m "<per-game mlc01>"
```

When fullscreen is enabled, `-f` is added.

## Diagnostics

The main report type is `CemuPreparationReport` in `include/skylanders/ProcessManager.h`.

It contains:

- applied global settings,
- selected/missing graphic packs,
- game profile path,
- controller profile paths,
- warnings.

The frontend reads the latest diagnostics from `nativeState.cemuPreparation`.

## Related Files

- `include/skylanders/ProcessManager.h`
- `src/ProcessManager.cpp`
- `include/skylanders/CemuConfigEditors.h`
- `src/CemuConfigEditors.cpp`
- `include/skylanders/GraphicPackManager.h`
- `src/GraphicPackManager.cpp`
- `docs/GraphicPackUpdateTask.md`
