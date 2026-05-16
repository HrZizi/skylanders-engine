# V2.2 Session Brief

Use this as the starting prompt/context for the next session. V2.1 is committed and verified; V2.2 should focus on launcher-owned native window/render ownership for embedded Cemu.

## Current Baseline

- Parent repo commit: `a700d7da Implement embedded Cemu readiness backend`
- Cemu submodule commit: `a7ded58 Separate headless embedded Cemu bootstrap`
- Default runtime remains bundled external Cemu.
- Experimental embedded mode is available behind `SKYLANDERS_EMBED_CEMU=ON`.
- V2.1 embedded mode configures Cemu, initializes bootstrap, prepares title diagnostics, and stops before visible gameplay rendering.
- Embedded mode intentionally reports `portalReady=false` and `inputBridgeReady=false`; direct integration comes after render ownership.
- Working tree should be clean except user-supplied untracked assets under `assets/characters/icons/*` and `assets/characters/renders/*`.

## V2.2 Goal

Make embedded Cemu render into a launcher-owned native host window/canvas, without starting external `Cemu.exe`.

The milestone is not Ultralight texture compositing yet. The existing external AppCore overlay may remain unavailable or diagnostic-only for embedded mode. V2.3 will render Ultralight into Cemu's Vulkan output.

## Primary Acceptance

- `SKYLANDERS_EMBED_CEMU=ON` build still configures and tests.
- Launching with `cemuBackendMode = embedded_native` creates one launcher process and no external `Cemu.exe`.
- The launcher owns the game HWND/canvas.
- At least one title reaches visible Cemu rendering in that launcher-owned window.
- Closing the launcher-owned game window shuts down embedded Cemu and exits cleanly.
- Resize/fullscreen lifecycle is routed through launcher-owned state, even if fullscreen remains experimental.
- External bundled Cemu path still works as the default fallback.

## First Implementation Steps

1. Read these files first:
   - `docs/V2_NATIVE_OVERLAY_ROADMAP.md`
   - `docs/PROJECT_MAP.md`
   - `docs/IMPLEMENTATION_STATUS.md`
   - `include/skylanders/EmbeddedCemuHost.h`
   - `src/EmbeddedCemuHost.cpp`
   - `extern/cemu/src/SkylandersEngineHeadlessWindowSystem.cpp`
   - `extern/cemu/src/gui/interface/WindowSystem.h`
   - `extern/cemu/src/gui/wxgui/wxWindowSystem.cpp`
   - `extern/cemu/src/Cafe/HW/Latte/Renderer/Vulkan/SwapchainInfoVk.cpp`
   - `extern/cemu/src/Cafe/HW/Latte/Renderer/Vulkan/VulkanRenderer.cpp`
2. Replace the V2.1 headless window shim with a launcher-provided window-system adapter for embedded mode.
3. Add a launcher `EmbeddedGameWindow` or similarly named owner for HWND creation, resize, close, focus, and fullscreen state.
4. Extend `EmbeddedCemuConfig` or a new runtime object with window/canvas handles and physical size.
5. Move `EmbeddedCemuHost::launch()` past `prepareTitle()` only when a valid render host exists.
6. Keep portal/input direct calls out of scope unless a tiny diagnostic hook is needed.

## Suggested Types

- `EmbeddedGameWindow`
  - owns HWND creation/destruction,
  - exposes main window/canvas handle,
  - tracks logical and physical size,
  - emits close/resize/focus events.
- `EmbeddedWindowSystemAdapter`
  - Cemu-side implementation of `WindowSystem::*` backed by launcher-provided state,
  - replaces `SkylandersEngineHeadlessWindowSystem.cpp` for the embedded render path.
- `EmbeddedCemuRenderHostConfig`
  - carries HWND/canvas handles, initial size, fullscreen preference, and Vulkan-only renderer requirement.

## Risk Notes

- Do not remove bundled external Cemu. It is still the default and safety net.
- Avoid pulling wx GUI libraries back into `SkylandersEngineTests`.
- Keep Cemu submodule changes small and separately committed.
- The first render path should require Vulkan. If Vulkan setup fails, report a clear diagnostic and fall back only when embedded was not explicitly selected.
- Input suppression and Ultralight compositing are V2.3/V2.4 concerns. V2.2 can keep overlay UI diagnostic-only for embedded mode.
- Be careful around Cemu globals and shutdown; `shutdown()` must remain repeatable after partial init.

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

If MSBuild fails because this sandbox exposes both `Path` and `PATH`, run:

```powershell
Remove-Item Env:PATH -ErrorAction SilentlyContinue
```

before build commands.

## Suggested Next-Session Prompt

```text
We are continuing Skylanders Engine at V2.2. V2.1 is committed and verified. Please read docs/V2_2_SESSION_BRIEF.md, docs/V2_NATIVE_OVERLAY_ROADMAP.md, docs/PROJECT_MAP.md, and docs/IMPLEMENTATION_STATUS.md first.

Goal: implement V2.2 native host window and Cemu render ownership for the experimental EmbeddedNative backend. Keep bundled external Cemu as the default fallback. Do not attempt Ultralight Vulkan compositing yet; that is V2.3. Keep Cemu submodule changes small and commit submodule and parent repo separately. Preserve untracked user asset folders.
```

## Documentation Rule

Any V2.2 code commit must update this file or retire it into completed notes, plus update:

- `docs/V2_NATIVE_OVERLAY_ROADMAP.md`
- `docs/IMPLEMENTATION_STATUS.md`
- `docs/PROJECT_MAP.md`
- `docs/ConsultantHandoffSummary.md`
