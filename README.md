# Skylanders Engine

Skylanders Engine is a Windows launcher/runtime that makes the Wii U Skylanders games feel closer to native PC releases. It wraps a project-owned Cemu build, resolves user-supplied game files and keys, applies per-game emulator settings, and exposes an Ultralight overlay for portal figure placement, controls, diagnostics, and setup.

The project does not ship copyrighted game files, title keys, or generated figure saves. Those stay in ignored local folders.

## Current Status

The project is in a playable external-Cemu milestone:

- Six Wii U Skylanders stubs are supported: Spyro's Adventure, Giants, Swap Force, Trap Team, SuperChargers, and Imaginators.
- The preferred emulator backend is the bundled `extern/cemu` submodule built as an external `Cemu.exe`.
- True in-process embedded Cemu is still reserved behind `SKYLANDERS_EMBED_CEMU` and is not the default.
- Ultralight starts hidden, boots Cemu automatically, and opens as an in-game overlay with `Start+Back` or `F1`.
- ROM discovery, key setup, portal figure creation/loading, controller mapping, and graphic-pack diagnostics are implemented.

See [docs/IMPLEMENTATION_STATUS.md](docs/IMPLEMENTATION_STATUS.md) for the detailed done/not-done list.

## Repository Layout

- `src/` and `include/skylanders/`: launcher/runtime C++ code.
- `assets/`: Ultralight HTML/CSS/JS and tracked UI assets.
- `extern/cemu/`: patched Cemu submodule used for the bundled backend.
- `tools/`: catalog, image import, and asset-gap scripts.
- `tests/`: lightweight C++ tests for catalog, locator, bridge, and config behavior.
- `docs/`: project map, handoff notes, generated manifests, and architecture docs.
- `roms/`: local user-supplied game files. Only `roms/README.md` is tracked.
- `keys/`: local user-supplied `keys.txt`. Only `keys/README.md` is tracked.
- `figures/`: generated `.sky` figure files and save state. Only `figures/README.md` is tracked.
- `runtime/`: local configs, downloads, diagnostics, MLC folders, logs, and controller profiles. Ignored.
- `sdks/ultralight/`: local Ultralight SDK. Ignored.

See [docs/PROJECT_MAP.md](docs/PROJECT_MAP.md) for feature ownership and data flows.

## First-Time Setup

1. Install Visual Studio with the C++ desktop workload and CMake support.
2. Initialize submodules if needed:

   ```powershell
   git submodule update --init --recursive
   ```

3. Put the Ultralight SDK at:

   ```text
   sdks/ultralight/
   ```

4. Put legally dumped Wii U game files in `roms/`, or choose them through the native file picker on first launch.
5. Put your own legally obtained Wii U keys in:

   ```text
   keys/keys.txt
   ```

6. Configure and build. The bundled build is the primary path:

   ```powershell
   cmake -S . -B build-bundled -DSKYLANDERS_BUILD_BUNDLED_CEMU=ON
   cmake --build build-bundled --config Release
   ```

   A non-bundled fallback build can be configured with:

   ```powershell
   cmake -S . -B build -DSKYLANDERS_BUILD_BUNDLED_CEMU=OFF
   cmake --build build --config Release
   ```

If MSBuild reports duplicate `Path`/`PATH` environment variables in this sandbox, run the build from a PowerShell session with the duplicate removed:

```powershell
Remove-Item Env:PATH -ErrorAction SilentlyContinue
cmake --build build-bundled --config Release
```

## Running

Launch one of the generated stubs from the Release output, for example:

```text
build-bundled/Release/Giants.exe
```

Normal boot should:

1. Resolve bundled Cemu.
2. Resolve the active game from the stub executable name.
3. Find a valid ROM in `roms/` or prompt for one.
4. Validate `keys/keys.txt`.
5. Prepare Cemu settings, game profile, controller profile, MLC path, and graphic-pack selections.
6. Launch Cemu.
7. Keep the Ultralight overlay hidden until toggled with `Start+Back` or `F1`.

## Verification

Recommended checks before handing off changes:

```powershell
node --check assets/app.js
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --build build-bundled --config Release
ctest --test-dir build-bundled -C Release --output-on-failure
```

## Documentation Rules

Documentation is part of the implementation. If a change modifies runtime behavior, public bridge APIs, config file formats, Cemu patch behavior, asset layout, or setup requirements, update the relevant docs in the same commit.

Use [docs/DOCUMENTATION_MAINTENANCE.md](docs/DOCUMENTATION_MAINTENANCE.md) as the maintenance checklist.
