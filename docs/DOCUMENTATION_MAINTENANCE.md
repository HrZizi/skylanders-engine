# Documentation Maintenance

Documentation should move with the code. A new contributor should be able to clone the repo, read `README.md`, and know where to continue.

## What To Update

Update documentation in the same commit when changing any of these:

- build flags, prerequisites, or setup paths,
- public C++ types in `include/skylanders/`,
- JavaScript bridge methods exposed from `src/main.cpp`,
- runtime config file names or schemas,
- Cemu command-line arguments, `settings.xml`, or game profile writes,
- Cemu submodule patch behavior,
- asset folder structure or naming rules,
- importer scripts or generated manifest formats,
- user-facing setup flow for ROMs, keys, figures, controls, or graphic packs.

## Where Things Belong

- `README.md`: first-run setup, build, run, verification, and high-level status.
- `docs/PROJECT_MAP.md`: architecture, file ownership, runtime flow, and data flow.
- `docs/IMPLEMENTATION_STATUS.md`: implemented features, missing work, deferred work, and known risks.
- `docs/ProcessManagerArchitecture.md`: Cemu launch/config preparation details.
- `docs/GraphicPackUpdateTask.md`: graphic-pack update/install behavior and troubleshooting.
- `assets/README.md`: asset naming, review folders, import scripts, and gap reports.
- `docs/ConsultantHandoffSummary.md`: concise current handoff for external reviewers or resumed agents.

Generated data files such as `docs/cemu-skylanders-catalog.json` and `docs/*manifest*.json` should be regenerated from tools rather than edited by hand unless a manual correction is explicitly being recorded.

## Before Committing

Run or intentionally skip with a note:

```powershell
node --check assets/app.js
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --build build-bundled --config Release
ctest --test-dir build-bundled -C Release --output-on-failure
```

For documentation-only edits, `node --check assets/app.js` and a targeted review may be enough, but the final handoff should say what was and was not run.

## Asset/Catalog Changes

When character data or assets change:

1. Regenerate the Cemu catalog if Cemu's `s_listSkylanders` changed.
2. Run import/assignment scripts as needed.
3. Regenerate the asset-gap report.
4. Confirm approved assets are tracked and `_review` candidates remain ignored.
5. Update `assets/README.md` if naming or folder rules changed.

## Cemu Submodule Changes

When editing `extern/cemu`:

1. Keep launcher-facing behavior documented in `docs/PROJECT_MAP.md`.
2. Keep Cemu-side IPC operations in sync with `PortalBridge` and `InputBridge`.
3. Commit the submodule changes inside `extern/cemu`.
4. Commit the updated submodule pointer in the parent repo.
5. Rebuild `build-bundled` and run its tests.

## Handoff Notes

When pausing the project for another worker, update:

- `docs/IMPLEMENTATION_STATUS.md` with what changed and what remains,
- `docs/ConsultantHandoffSummary.md` with the current recommended next step,
- `README.md` if setup or verification changed.

Prefer concrete file paths and exact commands over prose like "run the normal tests."
