# V2 Native Overlay Roadmap

V2 moves from the current bundled external Cemu process to a native in-process Cemu backend with Ultralight composited into Cemu's Vulkan output. The existing external backend stays available as a fallback until the native path is proven.

## Current Scaffolding

The launcher now has shared mode types and experimental host interfaces:

- `CemuBackendMode`: `BundledExternal`, `EmbeddedNative`, `ExternalPicked`
- `OverlayBackendMode`: `ExternalWindow`, `NativeRenderer`
- `EmbeddedCemuHost`
- `NativeOverlayRenderer`
- `InputRouter`

`runtime/config/setup.json` may persist backend selections, but the default remains the V1 external path.

The Cemu submodule also exposes a `CemuBootstrap` target with `SkylandersEngineBootstrapInitialize()` and `SkylandersEngineBootstrapShutdown()`. This is the first linkable split from Cemu's executable-only startup path.

Current verification note: `CemuBootstrap` and the embedded `skylanders_launcher` library compile. The full embedded executable/test link is still a V2.1 task because upstream Cemu libraries currently pull executable-only symbols and static-runtime dependencies that need to be separated cleanly.

## V2.1 Embedded Cemu Bootstrap

Goal: initialize Cemu in the launcher process without starting `Cemu.exe`.

Work:

- keep bundled external Cemu as the default fallback,
- make `SKYLANDERS_EMBED_CEMU=ON` build the launcher against `CemuBootstrap`,
- move more executable-only initialization behind linkable bootstrap APIs as needed,
- have `EmbeddedCemuHost` own initialization, MLC/config paths, profile loading, graphic packs, audio/input init, and title preparation,
- support repeated init/shutdown in one process.

Acceptance:

- embedded build configures and links,
- Cemu initialization reaches a controlled "ready to launch title" state,
- no external `Cemu.exe` process is started for the init smoke test.

## V2.2 Native Host Window And Render Ownership

Goal: Cemu renders into a launcher-owned native window/canvas.

Work:

- create the game host HWND in the launcher,
- add a Cemu window-system adapter for the launcher-owned Vulkan surface,
- route lifecycle, close, resize, fullscreen, crash, and diagnostics through the launcher,
- replace external focus juggling with a central `InputRouter`.

Acceptance:

- one game boots and renders in the launcher-owned window,
- closing the window closes the stub,
- no orphan Cemu process remains.

## V2.3 Ultralight Into Vulkan Output

Goal: draw the existing HTML/CSS/JS overlay into the game frame instead of a separate topmost window.

Work:

- replace AppCore windowing in embedded mode with Ultralight core `Renderer` and transparent `View`,
- render dirty Ultralight surfaces to a bitmap,
- upload the bitmap to a Vulkan texture,
- composite after the Cemu game frame and before presentation,
- rebuild overlay textures safely on resize/fullscreen/device loss,
- keep Vulkan as the required renderer for the first native overlay implementation.

Acceptance:

- opening the overlay draws the existing UI inside the game frame,
- fullscreen works,
- hidden overlay contributes no visible pixels,
- mouse, keyboard, and controller UI input reaches Ultralight.

## V2.4 Native Default And External Path Retirement

Goal: make embedded Cemu plus native overlay the normal release path.

Work:

- switch default backend to `EmbeddedNative` after V2.1-V2.3 pass,
- replace portal/input IPC with direct in-process calls,
- keep external IPC behind a documented fallback/debug backend,
- isolate old external-overlay positioning/focus code behind `ExternalOverlayBackend`,
- update packaging and docs around the new default.

Acceptance:

- release build boots all supported stubs,
- overlay opens/closes in-renderer,
- portal create/load/remove works through direct calls,
- controller mapping and overlay toggles persist,
- figures persist,
- fullscreen works,
- shutdown leaves no external process behind.

## Documentation Rule

Each V2 milestone must update this roadmap, `docs/IMPLEMENTATION_STATUS.md`, `docs/PROJECT_MAP.md`, and `docs/ConsultantHandoffSummary.md` in the same commit as the code change.
