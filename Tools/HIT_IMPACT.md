# Hit impact

Content Browser asset: `/Game/VFX/HitImpact/P_HitImpact` (Cascade Particle System).
Double-click it to view/edit the effect. Preview looping in Cascade is an editor
preview feature; the effect itself fires once and its last particles fade by 0.42 s.

Four layers, 28 particles per hit:

| Emitter | Appearance | Particle lifetime |
| --- | --- | --- |
| 01_ImpactFlash | Warm white central flash and four-point glint | 0.09 s |
| 02_ShockRing | Expanding thin gold shock ring | 0.20 s |
| 03_RadialShards | 16 gold directional sparks | 0.16–0.30 s |
| 04_EmberMotes | 10 small orange fading embers | 0.22–0.42 s |

Materials `M_HitFlash`, `M_HitRing` and `M_HitSpark` are self-contained procedural
additive sprite materials. Color is controlled by each emitter's Color Over Life
module. The effect is camera-facing, world-space, and contains no lights, sounds,
damage, collision or gameplay references.

## Gameplay integration

`ULOL_StatComponent::ApplyDamage` now sends one cosmetic multicast for each
positive, accepted damage event on the server. This shared path covers champions,
minions, jungle monsters and buildings, including both basic attacks and skills.
Zero damage, already-dead targets and hits rejected by existing friendly-fire or
dodge checks do not trigger it. Multi-part damage events each count as a hit.

Rendering clients spawn the one-shot at the character mesh bounds center (or the
actor location for buildings). It is not attached to the victim, so death does
not truncate the particles. Hidden targets and dedicated servers skip rendering.
The RPC is unreliable because this is a short-lived cosmetic effect.

Per-target StatComponent settings: `Combat | Hit VFX` → `bEnableHitImpact`,
`HitImpactScale` (default 1), `HitImpactEffect`. Damage amounts and timing are unchanged.

## Authoring and validation

`create_hit_impact_materials.py` creates/updates the three materials. After an
editor build, `build_hit_impact.py` invokes the editor-only authoring command
`LOL.VFX.BuildHitImpact` to generate the particle asset. This command is excluded
from non-editor builds and does not spawn anything in gameplay.

With a clean editor, `preview_hit_impact.py` creates an unsaved empty preview
scene; `test_hit_impact.py` checks the burst and captures four animation stages.
Results go to `Saved/DesignBackups/HitImpact*`. Stop preview and reload the original
map without saving the temporary scene.

For integration checks, run `prepare_hit_integration_test.py` in a clean editor
using standalone PIE, then `test_hit_integration.py` after play begins. It uses
real `TakeDamage` calls for champions, minions, a game-mode-spawned wolf and a
turret; checks rejected, hidden, disabled and lethal hits; and verifies automatic
particle cleanup. Results are written to `Saved/DesignBackups/HitIntegrationTest.json`.
These checks passed, as did the Editor build and Shipping build/full cook.
Two-player network delivery remains unverified by the automated test.
