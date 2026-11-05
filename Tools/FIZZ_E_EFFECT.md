# Fizz E water splash

E's first accepted cast emits a compact upward splash and 110 cm surface wave
at takeoff. `FinishPlayfulTrickster` emits a broader radial crown, ballistic
droplets, foam and a surface wave at the actual landing position. The latter
wave scales to `EDamageRadius` (currently 330 cm). Quick recasts still wait for
the existing landing callback; repeated descent input does not replay the burst.

`UFizzEWaterVisualComponent` owns presentation only: ground alignment, dynamic
ripple age, short particle lifetime, fog visibility, death and EndPlay cleanup.
Reliable multicast events pass the server's location and radius. Dedicated
servers skip effect creation. No targeting, damage, invulnerability, movement or
duration values are changed. Assets under `/Game/VFX/Fizz` are already cooked.

Rebuild with `Tools/create_fizz_e_material.py`, then (after building/restarting
the editor) `Tools/build_fizz_e_water.py` using `Tools/unreal.ps1 exec`.
`prepare_fizz_e_test.py` and `test_fizz_e_water.py` create an unsaved PIE fixture
and validate natural/recast timing, anchoring, radius scaling, particle spawning,
fog, death, EndPlay, damage to an inside target, a far outside target and cleanup.
Evidence is saved under `Saved/Validation/FizzE`. Restore the prior map after PIE.

## Existing damage-boundary limitation

The existing E sphere sweep uses `ECC_Pawn`; enemy `AttackRangeSphere` components
also overlap that channel and have a 400 cm radius. An enemy outside the nominal
330 cm damage radius can therefore be included through its detection sphere.
This was observed at 550 cm during visual testing. This effect-only change leaves
that gameplay logic untouched. The far-outside test uses 1420 cm separation;
it is not a precise boundary test. The surface wave depicts the configured
radius, not those extra detection spheres.
