# Ezreal R wave

The R projectile now uses a gold crescent with a narrow ivory leading rim,
animated cyan wake fragments, and world-space gold/cyan particles. It briefly
brightens on release and fades near its existing expiry time. The particle tail
drains after projectile destruction instead of disappearing in one frame.

Visual reference: [Riot's Ezreal champion page](https://www.leagueoflegends.com/en-us/champions/ezreal/).
This is an independently authored approximation using a procedural material,
the engine Plane mesh, and Cascade particles; no downloaded game assets.

## Editing

- Content Browser: `/Game/Level/ezreal/FX/R/M_EzrealRWave` and `P_EzrealRTrail`.
- Ezreal class defaults: `Ezreal | R VFX` → `RWaveWidth` (600 units),
  `RWaveIntensity` (1), `bEnableRWaveEffect` (true).
- Disable `bEnableRWaveEffect` to use the prior R mesh.
- The R particle template reuses the existing soft spark material `M_EzrealQTrail`;
  editing that shared material also affects Q, so duplicate it for R-specific changes.
- Collision radius, movement speed, travel time, damage, penetration, and cast
  timing are unchanged. Q/W visuals are unchanged.

`UEzrealRVisualComponent` owns the cosmetic material and particle tail. It is
created on each rendering client by the existing projectile multicast and is
skipped on dedicated servers. The existing collision actor still handles damage.

## Rebuilding the assets

1. Run `create_ezreal_r_material.py` in Unreal Python to create/update the material.
   It reuses existing expression nodes to avoid UE 5.7's rooted-expression delete assertion.
2. Compile the editor target and run `build_ezreal_r_trail.py` (command
   `LOL.Ezreal.BuildRTrail`). Restart the editor after first-time asset creation so
   native class defaults load the authored assets.
3. Both `.uasset` files must be included when sharing the source changes.

## Runtime validation

With no unsaved levels/assets, run `prepare_ezreal_r_preview.py`, then
`test_ezreal_r_visual.py`. These use a disposable unsaved level and the real
projectile multicast. The report and preview are written under
`Saved/DesignBackups`. Stop PIE and reopen the original map without saving the
temporary preview. The tests cover emission, movement, the unchanged collision
radius, early and natural cleanup, Q/W isolation, and the legacy visual toggle.
Network multiplayer behavior still needs a separate multi-client playtest.

Validated on UE 5.7.2: Editor Development and Win64 Shipping builds, full Win64
Shipping cook (0 errors; 2 pre-existing Data_ItemStats warnings), the R lifetime
test, and the Q regression test. Preview frames require completed material shader
compilation; a first uncooked-editor shot can precede shader warm-up.
