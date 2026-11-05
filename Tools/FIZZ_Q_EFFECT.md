# Fizz Q water spray

Q starts `P_FizzQWater` through a reliable multicast only after an accepted cast.
The local turquoise crest follows Fizz; world-space water ribbons, bilateral
ballistic droplets and foam remain briefly behind him. Emission ends with Q,
and the remaining particles fade for at most 0.5 seconds. Death and EndPlay
remove the component immediately. Visibility follows the champion; dedicated
servers skip presentation. Q movement, targeting and damage are unchanged.

Assets are in `/Game/VFX/Fizz`, explicitly included in packaging.

Rebuild assets in an editor with the current C++ module:

1. Run `Tools/create_fizz_q_materials.py` via `Tools/unreal.ps1 exec`.
2. Run `Tools/build_fizz_q_water.py` via the same bridge.

`prepare_fizz_q_test.py` opens an unsaved temporary map and starts PIE; it refuses
to run over dirty assets/maps. `test_fizz_q_water.py` checks actual Q casts,
unlearned/repeated-cast rejection, particles, target passthrough/damage, fog,
death, lost targets and residual-particle cleanup. Results and screenshots are
written to `LOL_UNREAL/Saved/Validation/FizzQ`. Stop PIE and reopen the previous
map without saving the temporary fixture afterwards.
