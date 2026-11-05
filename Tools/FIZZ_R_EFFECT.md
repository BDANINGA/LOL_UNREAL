# Fizz R: fin, water wake and shark warning

The former generic throw mesh is replaced with a swept, two-sided 3D dorsal fin,
a moving V-shaped foamy wake and world-space droplets. On arrival, a circular
water vortex marks the configured `RExplosionRadius` (default 300 cm). The fin
circles inside it and a second rim fills as the existing explosion timer expires.
The server's shark multicast removes the warning and emits a short water burst.
The original shark mesh and animation are retained.

`UFizzRWaterVisualComponent` handles cosmetics only. The existing sweep, hit
selection, travel time, damage, stun and detonation timer are unchanged. An attached
target moves the warning; misses remain at their endpoint. Destroyed targets fall
back to the same original explosion position used by the server. Destroying the
caster clears owned visuals. Dedicated servers skip visual creation.

The cosmetic multicast carries travel/arming time, target and fallback location.
The visibility flag follows the marked target, or the caster for an unmarked shot.
This does not introduce a separate position-based fog-of-war system. The existing
server radial sweep can include auxiliary pawn overlap components: the displayed
circle represents the configured radius, not a change to hit detection.

## Assets and regeneration

All new assets live under `/Game/VFX/Fizz`, already in `DirectoriesToAlwaysCook`:

- `SM_FizzRFin`: authored in `FizzRFinAssetBuilder.cpp`, forward +X, about 139 cm tall.
- `M_FizzRFin`: blue-grey fin with a bright leading edge.
- `M_FizzRWake`: animated surface wake.
- `M_FizzRWarning`: fixed boundary, spiral current and timed rim.
- `P_FizzRWake`: continuous droplets/foam, authored by `FizzWaterAssetBuilder.cpp`.
- The eruption reuses `P_FizzELanding` at the configured radius.

Build `LOL_UNREALEditor Win64 Development`, open the editor, then run sequentially:

```powershell
& Tools/unreal.ps1 exec -ScriptFile Tools/create_fizz_r_materials.py
& Tools/unreal.ps1 exec -ScriptFile Tools/build_fizz_r_water.py
```

`RProjectileScale`, `RProjectileVisualRadius` (relative to its default 70) and
`RProjectileRotationOffset` still adjust the fin. Collision and damage dimensions
remain independent of visual mesh size. Editor mesh-authoring dependencies are
enabled only for editor targets.

## Validation

With no dirty maps/assets and no PIE, run `prepare_fizz_r_test.py`, then
`test_fizz_r_water.py` through the bridge. The fixture is an unsaved blank map.
It learns R at level 6 and calls the real server skill path. Tests cover rejected
unlearned/duplicate casts, grounded travel, non-colliding visuals, moving targets,
hide/restore visibility, destroyed-target fallback, miss placement, 300 cm radius,
countdown progress, actual shark spawn, damage and component/particle cleanup.
Results and three rendered captures are in `Saved/Validation/FizzR`.

Stop PIE and restore `/Game/GameStart/GameStartMap` after inspection. This is
single-process PIE validation; multi-client networking and a fresh packaged
executable require separate validation. Repackage to update a distributed build.
