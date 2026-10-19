# Jax W weapon glow

W swaps only the skeletal mesh's `Weapon` material slot to a cached dynamic
instance of `/Game/VFX/Jax/M_JaxEmpowerWeapon`. The original texture and surface
shading remain, with amber emission, a pale gold edge and a brief ignition pulse.
The body is unchanged. A small camera-facing gold halo uses
`/Game/VFX/Jax/M_JaxEmpowerHalo`, attached to `Buffbone_Glb_Weapon_Loc` so it
follows the weapon animation. Both effects inherit the actor's fog visibility;
there are no detached particles or lights. No visual update runs while inactive.

`bWEmpowered` is replicated with `OnRep_WEmpowered`, including for clients that
gain relevance while W is active. Authority calls the same visual update locally.
Basic attacks, Q consumption, five-second expiry and death restore the exact
previous weapon material. W damage, mana cost and cooldown are unchanged.

Material authoring: `Tools/create_jax_w_material.py` (run in the full editor).
Brightness/color are in its custom expression; pulse strength is in
`AChampion_Jax::UpdateEmpowerVisual`.

Validation: Editor and Game Win64 Development builds passed. In a temporary PIE
fixture, actual W RPCs passed unlearned/recast rejection, body isolation, basic
attack and Q consumption, death, hidden-owner behavior, expiry and restoration.
Run `Tools/prepare_jax_w_test.py` from a clean editor, then run
`Tools/test_jax_w_visual.py` via `runpy.run_path` through the Python bridge.
Results and before/after screenshots are in `LOL_UNREAL/Saved/Validation/JaxW`.
Separate networked clients were not exercised. Existing packaged output must be
recooked/repackaged to include the new material and executable changes.
