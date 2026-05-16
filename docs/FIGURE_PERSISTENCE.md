# Figure Persistence

Generated Skylander files are runtime user state and live under:

```text
figures/<GameId>/<assetKey>.sky
```

Examples:

```text
figures/Giants/tree_rex.sky
figures/SwapForce/blast_zone_top.sky
figures/TrapTeam/life_hammer_trap.sky
```

## Ownership

Cemu is the authority for creating, loading, validating, and saving `.sky` files.

The launcher resolves a frontend `characterId` through the trusted catalog, computes the figure path, and sends `place_or_create` to the bundled Cemu portal bridge. Cemu then:

1. creates the missing `.sky` with `CreateSkylander(path, skyId, skyVar)`,
2. validates existing files against the expected `skyId` and `skyVar`,
3. refuses to overwrite wrong-ID files,
4. loads the file into the emulated portal,
5. reports success or failure to the launcher.

The UI only marks a slot as changed after Cemu returns success.

## Expected Save Timing

Cemu writes figure state when the emulated portal updates a figure block and when a loaded figure is removed or the title shuts down cleanly. For release verification, use Clear or close the game cleanly before judging whether gold/XP persisted.

If persistence appears broken:

- verify the same `figures/<GameId>/<assetKey>.sky` file timestamp changes,
- confirm the overlay did not create a duplicate asset key,
- confirm Cemu returned success for load/remove,
- confirm the test used the same game/stub and same catalog entry.

## Manual Persistence Test

1. Delete or move the target `.sky` file.
2. Launch a stub with bundled Cemu.
3. Open the overlay and place the character.
4. Confirm the `.sky` file appears under `figures/<GameId>/`.
5. Gain a visible amount of gold or XP.
6. Clear the portal slot or close the game cleanly.
7. Place the same character again.
8. Confirm gold/XP is retained.

This test should be recorded during V1.6 release smoke verification.
