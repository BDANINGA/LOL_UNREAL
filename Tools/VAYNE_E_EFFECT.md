# Vayne E — Condemn

## Assets

- `/Game/VFX/Vayne/SM_VayneCondemnBolt`: original 132 cm silver bolt with a broad 38 cm arrowhead and swept rear vanes; forward axis +X.
- `/Game/VFX/Vayne/M_VayneCondemnSilver`: faceted silver body.
- `/Game/VFX/Vayne/M_VayneCondemnGlow`: violet trail, white core and fading impact streaks.
- Rebuild with `Tools/create_vayne_e_assets.py` in Unreal Editor. Original OBJ is in `Tools/Art/Vayne`.

## Runtime

`AChampion_Vayne::Server_ExecuteCondemn` launches `AVayneCondemnProjectile` from the animated `r_hand` socket. The server applies the existing damage and fixed knockback once, on arrival, instead of immediately at cast time. Flight duration is distance / 2200, clamped to 0.09–0.24 seconds. This introduces a short travel delay; damage formula, 500 cm push distance, push duration and wall stun settings remain as before.

The bolt follows its selected target; it does not become a freely aimed skillshot. Dead or destroyed targets cancel the shot. Flight origin, target, launch time and impact phase replicate; only the server applies gameplay. Basic attacks and Q/W visuals are untouched.

The knockback component exposes a single-use wall collision callback. A real wall collision spawns a larger silver/violet burst. Ground or unit collisions do not trigger that burst. Impact actors expire and destroy their owned components; the runtime does not depend on Python or editor-only modules. Native hard references include the new assets in cooking.

## Validation

2026-11-15: Editor build passed. Both automation tests passed (0 failures). PIE confirmed flight, target impact, wall impact at x=729.59 before the wall at x=820, and complete effect cleanup. The temporary test scene was discarded after inspection.

- Editor target: `LOL_UNREALEditor Win64 Development`.
- Automation: `LOL.Combat.Vayne.CondemnProjectile` checks pre-arrival travel, moving targets, arrival damage/knockback, no duplicate damage and dead-target cancellation.
- Automation: `LOL.Combat.Vayne.FixedKnockback` checks fixed distance, long-frame collision, wall stun and one wall callback.
- `prepare_vayne_e_test.py` creates an unsaved temporary PIE scene. `test_vayne_e_visual.py` captures the flight, target impact and wall burst, and checks actor cleanup.
- Test outputs: `LOL_UNREAL/Saved/Validation/VayneE/` (not committed).
- A new packaged build and separate-machine multiplayer playtest are still required to validate the packaged/network environment.
