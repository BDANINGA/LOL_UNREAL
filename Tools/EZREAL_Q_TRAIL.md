# Ezreal Q particle trail

Q now emits short gold/cyan sparks behind the existing missile. World-space particles spread, shrink and fade over 0.18–0.42 seconds. The effect has no collision or damage and only spawns for Q on rendering clients. W/R behavior and the existing Q hit timing are unchanged.

Assets:
- `/Game/Level/ezreal/FX/P_EzrealQTrail`: two CPU sprite emitters, 150 gold + 55 cyan particles/second.
- `/Game/Level/ezreal/FX/M_EzrealQTrail`: soft, additive, unlit particle material.

Tune the champion's **Ezreal | Q Trail** properties: `bEnableQTrail`, `QTrailScale` (default 1), and `QTrailEffect`. Edit the particle asset's rate, velocity and lifetime for denser or wider scattering.

`UEzrealQTrailComponent` owns an independent cosmetic emitter. When the missile expires or is destroyed, it detaches and stops spawning; existing particles fade before the emitter is destroyed. A bounded lifespan also handles unexpected teardown. The existing projectile multicast creates the local trail on each client; the emitter itself does not replicate and is skipped on dedicated servers.

Authoring: run `create_ezreal_q_material.py`, then `build_ezreal_q_trail.py` through `Tools/unreal.ps1 exec -ScriptFile ...`. The latter invokes the editor-only `LOL.Ezreal.BuildQTrail` console command. These tools rebuild only the new Q effect assets; running them overwrites manual edits to those assets.

Runtime check: `prepare_ezreal_q_preview.py` creates an unsaved temporary level (requires no dirty maps/assets), then `test_ezreal_q_trail.py` invokes the actual projectile multicast. It checks emission and movement, forced-destruction tail survival/cleanup, natural expiration, W/R exclusion and the disabled-Q setting. Results and a screenshot are in `Saved/DesignBackups/EzrealQTrailTest.json` and `EzrealQTrail_Preview.png`. Multiplayer was not exercised in this preview.

Validated on 2026-10-06: editor build and the runtime checks passed; Win64 Shipping build and full cook passed (0 errors, 2 pre-existing missing Data_ItemStats warnings). Both new assets are included in the cooked output. Build/cook log: `Saved/Logs/EzrealQTrail_ShippingCook.log`. Final staging/packaging and a packaged-game launch were not performed.
