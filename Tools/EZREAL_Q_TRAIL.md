# Ezreal Q and W particle trails

Q and W emit gold/cyan sparks behind their existing missiles. World-space particles spread, shrink and fade over 0.24–0.52 seconds. The effect has no collision or damage and only spawns on rendering clients. Projectile movement, hit timing and R visuals are unchanged.

Assets:
- `/Game/Level/ezreal/FX/P_EzrealQTrail`: two CPU sprite emitters, 400 gold + 150 cyan particles/second (previously 150 + 55). Gold sprites are 6.5–13 units, cyan sprites 4–8 units before per-skill scaling. Slightly wider scattering and longer lifetime make the trail easier to see.
- `/Game/Level/ezreal/FX/M_EzrealQTrail`: soft, additive, unlit particle material.

Tune the champion's **Ezreal | Q Trail** properties: `bEnableQTrail`, `QTrailScale` (default 1), and `QTrailEffect`. Edit the particle asset's rate, velocity and lifetime for denser or wider scattering.

W reuses the same asset with its own **Ezreal | W Trail** properties:
`bEnableWTrail`, `WTrailScale` (default 0.8), and `WTrailEffect`. The two skills can
be enabled and scaled independently. Editing the shared particle asset affects
both; assign a separate asset to `WTrailEffect` for independent particle authoring.

`UEzrealQTrailComponent` owns an independent cosmetic emitter. When the missile expires or is destroyed, it detaches and stops spawning; existing particles fade before the emitter is destroyed. A bounded lifespan also handles unexpected teardown. The existing projectile multicast creates the local trail on each client; the emitter itself does not replicate and is skipped on dedicated servers.

Authoring: run `create_ezreal_q_material.py`, then `build_ezreal_q_trail.py` through `Tools/unreal.ps1 exec -ScriptFile ...`. The latter invokes the editor-only `LOL.Ezreal.BuildQTrail` console command. These tools rebuild only the new Q effect assets; running them overwrites manual edits to those assets.

Runtime check: `prepare_ezreal_q_preview.py` creates an unsaved temporary level (requires no dirty maps/assets), then `test_ezreal_q_trail.py` invokes the actual projectile multicast. Run `test_ezreal_w_trail.py` in a fresh preview session for W. These check emission, movement and scale, forced-destruction tail survival/cleanup, natural expiration, independent skill toggles and R exclusion. Results and screenshots are in `Saved/DesignBackups/EzrealQTrailTest.json`, `EzrealWTrailTest.json` and the corresponding `EzrealQTrail_Preview.png` / `EzrealWTrail_Preview.png`. Multiplayer was not exercised in these previews.

Validated on 2026-10-06: editor build and the runtime checks passed; Win64 Shipping build and full cook passed (0 errors, 2 pre-existing missing Data_ItemStats warnings). Both new assets are included in the cooked output. Build/cook log: `Saved/Logs/EzrealQTrail_ShippingCook.log`. Final staging/packaging and a packaged-game launch were not performed.

W integration on 2026-10-09: Editor and Win64 Shipping builds passed. W flight,
0.8 scale, independent toggles, forced-destruction fade-out and natural expiration
passed in PIE. This change reuses the existing assets; no asset rebuild is required.

Density follow-up on 2026-10-09: rebuilt the shared particle asset with 550 total
particles/second and 0.24–0.52 s lifetime. Editor build and both PIE trail checks
passed, including tail cleanup. At the flight checkpoint Q/W had 221/225 active
particles (previously 68/67); screenshots were reviewed for visibility.
