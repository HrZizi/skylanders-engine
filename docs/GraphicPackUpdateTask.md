# Graphic Pack Update And Diagnostics

Graphic packs are managed by `GraphicPackManager` and selected by `ProcessManager`.

## Implemented Behavior

- The launcher can download the current version-6 ZIP linked by the Cemu graphic packs website.
- Downloads are cached under `runtime/downloads/`.
- Extracted packs are installed into `<cemuRoot>/graphicPacks`.
- Existing unrelated/custom pack folders are preserved.
- Update state is stored in `runtime/config/graphic_packs.json`.
- Auto-check runs at most once every 7 days.
- The overlay exposes a manual "Update Graphic Packs" action.
- Download or extraction failure is non-fatal; the game still launches with a warning.

Primary files:

- `include/skylanders/GraphicPackManager.h`
- `src/GraphicPackManager.cpp`
- `src/ProcessManager.cpp`
- `assets/app.js`

## Launch Selection

`ProcessManager` scans `cemuRoot/graphicPacks` for `rules.txt` folders and records each attempted selection in `CemuPreparationReport`.

Current defaults:

- all games: resolution preset, default `2560x1440`,
- Giants: optional FPS preset plus optional Vulkan/stability fixes,
- Swap Force: portal/stability and Vulkan fixes when present,
- Trap Team: portal/traptanium and Vulkan fixes when present,
- SuperChargers and Imaginators: Vulkan fixes when present.

Missing important packs become warnings. Missing optional packs are reported but do not block launch.

## Cemu XML Format

Cemu expects graphic-pack presets as nested elements, not attributes:

```xml
<GraphicPack>
  <Entry filename="graphicPacks/Enhancements/Resolution/rules.txt">
    <Preset>
      <category>Resolution</category>
      <preset>2560x1440</preset>
    </Preset>
  </Entry>
</GraphicPack>
```

If graphic packs appear downloaded but not applied, inspect:

- `runtime/config/graphic_packs.json`
- `runtime/config/last_launch_diagnostics.json`
- `<cemuRoot>/settings.xml`
- `<cemuRoot>/graphicPacks/**/rules.txt`

## Maintenance Notes

- Keep failures non-fatal. ROM/key launch should not depend on network access.
- If Cemu changes the graphic-pack rules or preset XML shape, update `CemuConfigEditors`.
- If the Cemu website changes its ZIP link format, update `GraphicPackManager::findVersion6ZipUrl` and its tests.
