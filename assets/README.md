# Overlay Assets

The Ultralight overlay loads assets relative to this `assets/` folder.

## Backgrounds

- `backgrounds/green-portal.png` is the active character selector background.
- Replace it with another PNG/JPG using the same filename to change the backdrop without code changes.

## Character Renders

Approved, tracked renders should live at:

```text
characters/renders/<GameId>/<assetKey>.png
```

Examples:

```text
characters/renders/SpyrosAdventure/spyro.png
characters/renders/Giants/tree_rex.png
characters/renders/SwapForce/blast_zone_top.png
```

The UI falls back to review candidates at:

```text
characters/renders/_review/<GameId>/<assetKey>.(png|jpg|jpeg|webp)
```

The `_review` folder is ignored by git and is meant for downloaded candidates that still need approval.
The `Downloads` folder inside `_review` is treated as an unassigned inbox. Move manually collected renders into the canonical review layout with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\assign_local_renders.ps1
```

Run this to refresh review candidates from the generated Cemu catalog:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\import_character_images.ps1
```

## Game Logos

Preferred game logos should live at:

```text
game-logos/Skylanders_*.webp
```

Expected filenames:

```text
game-logos/Skylanders_Spyros_Adventure.webp
game-logos/Skylanders_Giants.webp
game-logos/Skylanders_Swap_Force.webp
game-logos/Skylanders_Trap_Team.webp
game-logos/Skylanders_Super_Chargers.webp
game-logos/Skylanders_Imaginators.webp
```

The UI also checks the old `<GameId>.png` names, then shows a styled text fallback if no logo loads.

## Character Wheel Icons

Approved wheel icons should live at:

```text
characters/icons/<GameId>/<assetKey>.png
```

Downloaded review candidates from the Skylanders Wiki importer live at:

```text
characters/icons/_review/<GameId>/<assetKey>.(png|jpg|jpeg|webp)
```

Run this to refresh review candidates:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\import_wiki_icons.ps1
```

Wheel cards load images in this order: approved icon, review icon, approved render, review render, base-variant icon/render, placeholder.
Large character renders use the exact render first, then the base variant render, then the placeholder.

## Cemu Catalog And Gap Report

The supported toy list is generated from Cemu's local Skylander creation table:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\sync_cemu_skylanders_catalog.ps1
```

The generator writes:

```text
docs/cemu-skylanders-catalog.json
src/GeneratedCharacterCatalog.inc
```

After refreshing render/icon candidates, generate the manual follow-up report with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\write_asset_gap_report.ps1
```

The report lists supported Cemu entries still missing a render, an icon, or both.

## Element Icons

Approved element icons should live at:

```text
elements/<element>.png
```

Expected lowercase element names:

```text
air.png
dark.png
earth.png
fire.png
kaos.png
life.png
light.png
magic.png
tech.png
undead.png
water.png
```

Downloaded review candidates from the Skylanders Wiki importer live at:

```text
elements/_review/<element>.(png|jpg|jpeg|webp)
```

Run this to refresh review candidates:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\import_element_icons.ps1
```
