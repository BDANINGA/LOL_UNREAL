# Fizz W water strike

- `P_FizzWWeapon`: compact water sheets, droplets and foam attached to
  `BUFFBONE_CSTM_WEAPON_1`. The body's shared material is preserved.
- `P_FizzWImpact`: one-shot water cut, ballistic spray and expanding foam at
  the target's body when an empowered hit is consumed, including Q on-hit.
- `bWEmpowered` replicates with an OnRep callback. The weapon effect stops on
  consumption, timeout, death and EndPlay. It follows the champion's visibility;
  dedicated servers skip cosmetic creation. Impact particles auto-destroy.
- Damage values, passive bleed, Q movement and W duration are unchanged.

All assets are in `/Game/VFX/Fizz`, already included in packaging by the Q work.
After building/restarting the editor, run `Tools/build_fizz_w_water.py` through
`Tools/unreal.ps1 exec` to rebuild the W assets. It reuses Q's water materials.

`prepare_fizz_w_test.py` creates an unsaved PIE scene; `test_fizz_w_water.py`
checks actual W casts and basic-attack input, weapon-bone attachment, duplicate
and unlearned rejection, Q consumption, visibility, death, timeout, one-shot
impact particles and cleanup. Evidence goes to `Saved/Validation/FizzW`.
Stop PIE and reopen the prior map without saving the temporary scene afterwards.
