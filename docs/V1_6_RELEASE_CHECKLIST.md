# V1.6 Release Checklist

V1.6 is the final external-Cemu-first baseline. The goal is a clean folder that a fresh user can run after adding their own ROMs and keys.

## Build And Package

From the repository root:

```powershell
cmake -S . -B build-bundled -DSKYLANDERS_BUILD_BUNDLED_CEMU=ON
cmake --build build-bundled --config Release
.\tools\package_release.ps1 -BuildDir build-bundled -Configuration Release
```

The package is written under `out/releases/SkylandersEngine-<timestamp>/`.

## Package Contents

Required:

- `SkylandersEngine.exe`
- all six game stubs
- `cemu/Cemu.exe`
- `cemu/resources/`
- `cemu/gameProfiles/`
- Ultralight DLLs/resources copied beside the stubs
- `assets/` overlay UI and tracked images
- `README.md`
- `docs/`
- empty `roms/`, `keys/`, and `figures/` folders with README files
- empty `runtime/config/` and `runtime/controllerProfiles/` placeholders

Must not be included:

- ROMs
- title keys
- generated `.sky` figures
- local `runtime/` state
- build symbols such as `.pdb` and `.ilk`

## Static Smoke Check

Run:

```powershell
.\tools\smoke_check_release.ps1 -PackagePath out\releases\SkylandersEngine-<timestamp>
```

The smoke script checks required files and runs `node --check assets/app.js` when Node is available.

## Manual Smoke Check

1. Put a valid game file in `roms/` or launch a stub and select one with the native picker.
2. Put user-supplied `keys.txt` in `keys/`.
3. Launch `Giants.exe`.
4. Confirm bundled Cemu is used and no Cemu path picker appears.
5. Confirm missing graphic packs only produce a warning and do not block launch.
6. Toggle the overlay with `F1`.
7. Place a figure that does not exist yet and confirm Cemu creates it under `figures/<GameId>/`.
8. Collect a small amount of gold or XP, clear/remove the figure, place it again, and confirm state persisted.
9. Open Controls and confirm Player 1 device scan results appear without pressing Refresh.
10. Map one keyboard input and one controller input if hardware is available.
11. Add a custom overlay toggle, release the buttons, then confirm the new toggle works without closing the overlay during capture.
12. Run Update Graphic Packs in diagnostics and confirm success or a non-fatal error is reported.
13. Close Cemu and confirm the stub process exits.

## Verification Notes

Record the exact commands run and any manual hardware used in the release commit or handoff notes. If a manual step is skipped, say why.
